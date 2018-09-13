struct MontgomeryData
{
	DWORD K;			/* length of modulus */
	DWORD M0Prime;  	/* -M[0]**(-1) mod 2**DIGIT_BITS */
	LPDWORD M;      	/* modulus */
	LPDWORD product;	/* space for temporary product */
};

void MontgomerySetup(struct MontgomeryData *context, LPDWORD M, DWORD N);
void MontgomeryTeardown(struct MontgomeryData *context);
void MontgomeryTransform(struct MontgomeryData *context, LPDWORD X);
void MontgomeryReduce(struct MontgomeryData *context, LPDWORD T, LPDWORD X);
void MontgomeryModSquare(struct MontgomeryData *context, LPDWORD A, LPDWORD B);
void MontgomeryModMultiply(struct MontgomeryData *context, LPDWORD A, LPDWORD B, LPDWORD C);
void MontgomeryModExp(LPDWORD A, LPDWORD B, LPDWORD C, LPDWORD D, DWORD len);
void MontgomeryModRoot(LPDWORD M, LPDWORD C, LPDWORD PP, LPDWORD QQ, LPDWORD DP, LPDWORD DQ, LPDWORD CR, DWORD PSize);
