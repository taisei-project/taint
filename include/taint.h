
#ifndef TAINT_H
#define TAINT_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined TAINT_BUILDING_SHARED && defined __GNUC__ && !defined _WIN32
    #define TAINT_API __attribute__((visibility("default")))
#elif defined TAINT_BUILDING_SHARED && defined _WIN32
    #define TAINT_API __declspec(dllexport)
#else
    #define TAINT_API
#endif

#define TAINT_MAKE_VERSION(major, minor, patch) \
    ((major) * 1000000 + (minor) * 1000 + (patch))

#define TAINT_VERSION_MAJOR 0
#define TAINT_VERSION_MINOR 0
#define TAINT_VERSION_PATCH 0

#define TAINT_VERSION TAINT_MAKE_VERSION(TAINT_VERSION_MAJOR, TAINT_VERSION_MINOR, TAINT_VERSION_PATCH)

#define TAINT_VERSION_EXTRACT_MAJOR(version) ((version) / 1000000)
#define TAINT_VERSION_EXTRACT_MINOR(version) (((version) / 1000) % 1000)
#define TAINT_VERSION_EXTRACT_PATCH(version) ((version) % 1000)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum TaintOption {
    TAINT_OPT_WGSL_ALLOW_NON_UNIFORM_DERIVATIVES          = 0,
    TAINT_OPT_WGSL_ALLOW_NON_UNIFORM_SUBGROUP_OPERATIONS  = 1,
    TAINT_OPT_WGSL_DISABLE_UNREACHABLE_CODE_WARNING       = 2,
    TAINT_OPT_WGSL_MINIFY                                 = 3,
} TaintOption;

typedef struct TaintBindingPoint {
    uint32_t group;     // SPIR-V DescriptorSet
    uint32_t binding;
} TaintBindingPoint;

typedef struct TaintSamplerMapping {
    TaintBindingPoint from, to;
} TaintSamplerMapping;

typedef struct TaintOptions TaintOptions;
typedef struct TaintModule TaintModule;

// Use TAINT_VERSION_EXTRACT_* macros to convert into the major.minor.patch format.
TAINT_API uint32_t taint_library_version(void);

// Returns NULL on allocation failure
TAINT_API TaintOptions *taint_options_create(void);
TAINT_API void taint_options_destroy(TaintOptions *options);

// All setters return false if an argument is invalid, true otherwise.

TAINT_API bool taint_options_set_bool(TaintOptions *options, TaintOption option, bool value);

// Names as they appear in WGSL `enable` and `requires` directives.
// Refer to src/tint/lang/wgsl/enums.h in tint's source for the full list it currently supports.
TAINT_API bool taint_options_allow_wgsl_extension(TaintOptions *options, const char *extension);
TAINT_API bool taint_options_allow_wgsl_feature(TaintOptions *options, const char *feature);

TAINT_API bool taint_options_allow_all_wgsl_extensions(TaintOptions *options);
TAINT_API bool taint_options_allow_all_wgsl_features(TaintOptions *options);

// WGSL has no combined image-samplers, so if the SPIR-V uses any, tint will split them into
// texture/sampler pairs. If you know the binding points of your combined samplers, you can tell
// tint where to put the samplers by passing a mapping for each combined sampler. The `to` field
// of the mapping is the binding point of the sampler. The `from` field is the binding point of
// the combined sampler. Unmapped samplers will get the next free binding in their group.
//
// If there are no mappings, tint will put every sampler after its corresponding texture. Any
// existing bindings that collide with the new assignments will be shifted.
//
// Example (glsl):
//
// layout(set = 0, binding = 0) uniform sampler2D combinedSampler0;
// layout(set = 0, binding = 1) uniform sampler2D combinedSampler1;
// layout(set = 0, binding = 2) buffer storageBuffer0;
// layout(set = 0, binding = 3) buffer storageBuffer1;
//
// will be translated into something like (wgsl):
//
// @group(0) @binding(0) var combinedSampler0_image: texture_2d<f32>;
// @group(0) @binding(1) var combinedSampler0_sampler: sampler;
// @group(0) @binding(2) var combinedSampler1_image: texture_2d<f32>;
// @group(0) @binding(3) var combinedSampler1_sampler: sampler;
// @group(0) @binding(4) var<storage, ...> storageBuffer0: ...;
// @group(0) @binding(5) var<storage, ...> storageBuffer1: ...;
//
// Note that this matches SDL_GPU's expectations, so you can leave the mappings alone if that
// is what you're using.
TAINT_API bool taint_options_set_sampler_mappings(
    TaintOptions *options, size_t num_mappings, const TaintSamplerMapping *mappings);

// Attempts to translate the given SPIR-V module to WGSL.
// Note that tint sucks and may crash your process if it doesn't particularly like your input.
//
// Returns NULL on allocation failure.
// A TaintModule is returned even if the translation has failed.
// Use taint_module_get_wgsl and taint_module_get_diagnostics to query the result.
//
// `options` can be NULL, but if passed, it must not be modified while this function runs.
// It can be disposed of after it returns.
TAINT_API TaintModule *taint_spirv_to_wgsl(size_t spirv_words, const uint32_t *spirv, const TaintOptions *options);

// These return a null-terminated string or NULL. `out_size` is optional; if passed it will be set to
// the size of the string *without* the null-terminator.
// The returned strings are owned by the module and only valid until the latter is destroyed.

// If the translation succeeded, returns the WGSL source; otherwise NULL.
TAINT_API const char *taint_module_get_wgsl(const TaintModule *module, size_t *out_size);

// If the translation succeeded, returns non-fatal warnings.
// Otherwise, returns error messages.
// May still return NULL if module is invalid, or if Mercury is in retrograde or whatever.
TAINT_API const char *taint_module_get_diagnostics(const TaintModule *module, size_t *out_size);

// Frees all memory associated with the module. The pointer becomes invalid after this call.
TAINT_API void taint_module_destroy(TaintModule *module);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // TAINT_H
