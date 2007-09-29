/* Fax G3/G4 tables */

typedef struct cfe_code_s cfe_code;

struct cfe_code_s
{
	unsigned short code;
	unsigned short nbits;
};

typedef struct cf_runs_s {
	cfe_code termination[64];
	cfe_code makeup[41];
} cf_runs;

/* Encoding tables */

/* Codes common to 1-D and 2-D encoding. */
extern const cfe_code cf_run_eol;
extern const cf_runs cf_white_runs, cf_black_runs;
extern const cfe_code cf_uncompressed[6];
extern const cfe_code cf_uncompressed_exit[10]; /* indexed by 2 x length of */

/* 1-D encoding. */
extern const cfe_code cf1_run_uncompressed;

/* 2-D encoding. */
enum { cf2_run_vertical_offset = 3 };
extern const cfe_code cf2_run_pass;
extern const cfe_code cf2_run_vertical[7]; /* indexed by b1 - a1 + offset */
extern const cfe_code cf2_run_horizontal;
extern const cfe_code cf2_run_uncompressed;

/* 2-D Group 3 encoding. */
extern const cfe_code cf2_run_eol_1d;
extern const cfe_code cf2_run_eol_2d;

