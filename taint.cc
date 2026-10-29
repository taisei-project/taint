
#include <taint.h>

#include <cstring>
#include <new>
#include <span>
#include <string>

#include "tint/api/common/binding_point.h"
#include "tint/lang/core/ir/module.h"
#include "tint/lang/core/ir/validator/validate.h"
#include "tint/lang/spirv/reader/common/options.h"
#include "tint/lang/spirv/reader/lower/lower.h"
#include "tint/lang/spirv/reader/parser/parser.h"
#include "tint/lang/spirv/reader/reader.h"
#include "tint/lang/wgsl/enums.h"
#include "tint/lang/wgsl/program/program.h"
#include "tint/lang/wgsl/writer/common/options.h"
#include "tint/lang/wgsl/writer/output.h"
#include "tint/lang/wgsl/writer/writer.h"
#include "tint/utils/result.h"

struct TaintOptions {
    tint::spirv::reader::Options spirv_opts;
    tint::wgsl::writer::Options wgsl_opts;
};

struct TaintModule {
    // program is useful to retain if we ever want to add reflection
    tint::Result<tint::Program> program = tint::Failure();
    tint::Result<tint::wgsl::writer::Output> wgsl = tint::Failure();
    std::string diagnostics;
};

extern "C" {

TAINT_API uint32_t taint_library_version(void) {
    return TAINT_VERSION;
}

TAINT_API TaintModule *taint_spirv_to_wgsl(
    size_t spirv_words, const uint32_t *spirv, const TaintOptions *options
) {
    static const TaintOptions default_options;
    const TaintOptions& opts = options ? *options : default_options;

    auto module = new(std::nothrow) TaintModule;

    if(module == nullptr) {
        return nullptr;
    }

    auto parse_result = tint::spirv::reader::Parse(std::span { spirv, spirv_words }, opts.spirv_opts);

    if(parse_result != tint::Success) {
        module->diagnostics = parse_result.Failure().reason;
        return module;
    }

    auto ir = parse_result.Move();
    ir.dump_ir_when_validating = opts.spirv_opts.dump_ir_when_validating;
    ir.enable_validation_asserts = opts.spirv_opts.enable_validation_asserts;

    auto lower_result = tint::spirv::reader::Lower(ir);

    if(lower_result != tint::Success) {
        module->diagnostics = lower_result.Failure().reason;
        return module;
    }

    auto validation_result = tint::core::ir::Validate(ir);

    if(validation_result != tint::Success) {
        module->diagnostics = validation_result.Failure().reason;
        return module;
    }

    module->program = tint::wgsl::writer::ProgramFromIR(ir, opts.wgsl_opts);

    if(module->program != tint::Success) {
        module->diagnostics = module->program.Failure().reason;
        return module;
    }

    module->diagnostics = module->program.Get().Diagnostics().Str();
    module->wgsl = tint::wgsl::writer::Generate(module->program.Get(), opts.wgsl_opts);

    if(module->wgsl != tint::Success) {
        module->diagnostics = module->wgsl.Failure().reason;
        return module;
    }

    return module;
}

TAINT_API void taint_module_destroy(TaintModule *module) {
    delete module;
}

TAINT_API const char *taint_module_get_diagnostics(const TaintModule *module, size_t *out_size) {
    if(module == nullptr) {
        return nullptr;
    }

    if(out_size != nullptr) {
        *out_size = module->diagnostics.size();
    }

    return module->diagnostics.c_str();
}

TAINT_API const char *taint_module_get_wgsl(const TaintModule *module, size_t *out_size) {
    if(module == nullptr || module->wgsl != tint::Success) {
        return nullptr;
    }

    auto &wgsl = module->wgsl.Get().wgsl;

    if(out_size != nullptr) {
        *out_size = wgsl.size();
    }

    return wgsl.c_str();
}

TAINT_API TaintOptions *taint_options_create(void) {
    return new(std::nothrow) TaintOptions;
}

TAINT_API void taint_options_destroy(TaintOptions *options) {
    delete options;
}

TAINT_API bool taint_options_set_bool(TaintOptions *options, TaintOption option, bool value) {
    if(options == nullptr) {
        return false;
    }

    switch(option) {
    case TAINT_OPT_WGSL_ALLOW_NON_UNIFORM_DERIVATIVES:
        options->wgsl_opts.allow_non_uniform_derivatives = value;
        return true;

    case TAINT_OPT_WGSL_ALLOW_NON_UNIFORM_SUBGROUP_OPERATIONS:
        options->wgsl_opts.allow_non_uniform_subgroup_operations = value;
        return true;

    case TAINT_OPT_WGSL_DISABLE_UNREACHABLE_CODE_WARNING:
        options->wgsl_opts.disable_unreachable_code_warning = value;
        return true;

    case TAINT_OPT_WGSL_MINIFY:
        options->wgsl_opts.minify = value;
        return true;
    }

    return false;
}

TAINT_API bool taint_options_allow_wgsl_extension(TaintOptions *options, const char *extension) {
    if(options == nullptr || extension == nullptr) {
        return false;
    }

    auto ext = tint::wgsl::ParseExtension(extension);

    if(ext == tint::wgsl::Extension::kUndefined) {
        return false;
    }

    options->wgsl_opts.allowed_features.extensions.insert(ext);
    return true;
}

TAINT_API bool taint_options_allow_wgsl_feature(TaintOptions *options, const char *feature) {
    if(options == nullptr || feature == nullptr) {
        return false;
    }

    auto feat = tint::wgsl::ParseLanguageFeature(feature);

    if(feat == tint::wgsl::LanguageFeature::kUndefined) {
        return false;
    }

    options->wgsl_opts.allowed_features.features.insert(feat);
    return true;
}

TAINT_API bool taint_options_allow_all_wgsl_extensions(TaintOptions *options) {
    if(options == nullptr) {
        return false;
    }

    for(auto ext : tint::wgsl::kAllExtensions) {
        options->wgsl_opts.allowed_features.extensions.insert(ext);
    }

    return true;
}

TAINT_API bool taint_options_allow_all_wgsl_features(TaintOptions *options) {
    if(options == nullptr) {
        return false;
    }

    for(auto feat : tint::wgsl::kAllLanguageFeatures) {
        options->wgsl_opts.allowed_features.features.insert(feat);
    }

    return true;
}

TAINT_API bool taint_options_set_sampler_mappings(
    TaintOptions *options, size_t num_mappings, const TaintSamplerMapping *mappings
) {
    if(options == nullptr || (mappings == nullptr && num_mappings != 0)) {
        return false;
    }

    options->spirv_opts.sampler_mappings.clear();

    for(size_t i = 0; i < num_mappings; ++i) {
        auto m = &mappings[i];
        tint::BindingPoint from = { m->from.group, m->from.binding };
        tint::BindingPoint to   = { m->to.group,   m->to.binding };
        options->spirv_opts.sampler_mappings[from] = to;
    }

    return true;
}

}  // extern "C"
