
#include <stdlib.h>
#include <sys/ucontext.h>
#include <taint.h>

#include <stdio.h>

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	size_t buf_size = 4096;
	size_t buf_fill = 0;
	char *buf = malloc(buf_size);

	while(!feof(stdin)) {
		if(buf_size - buf_fill < 1) {
			buf_size *= 2;
			buf = realloc(buf, buf_size);
		}

		size_t read_size = fread(buf + buf_fill, 1, buf_size - buf_fill, stdin);
		buf_fill += read_size;
	}

	TaintOptions *opts = taint_options_create();
	taint_options_set_bool(opts, TAINT_OPT_WGSL_ALLOW_NON_UNIFORM_DERIVATIVES, true);
	// taint_options_set_bool(opts, TAINT_OPT_WGSL_MINIFY, true);

	int exit_code = 0;

	TaintModule *module = taint_spirv_to_wgsl(buf_fill / sizeof(uint32_t), (uint32_t*)buf, opts);
	taint_options_destroy(opts);

	if(!module) {
		fprintf(stderr, "taint_spirv_to_wgsl() failed\n");
		return 1;
	}

	size_t diag_size;
	const char *diag = taint_module_get_diagnostics(module, &diag_size);

	if(diag) {
		fwrite(diag, 1, diag_size, stderr);
		fputc('\n', stderr);
		fflush(stderr);
	}

	size_t wgsl_size;
	const char *wgsl = taint_module_get_wgsl(module, &wgsl_size);

	if(wgsl) {
		fwrite(wgsl, 1, wgsl_size, stdout);
		fputc('\n', stdout);
		fflush(stdout);
	} else {
		exit_code = 1;
	}

	taint_module_destroy(module);
	return exit_code;
}
