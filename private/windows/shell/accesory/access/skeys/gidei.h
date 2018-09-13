/* GIDEI Codes */

/* reserved 236-242 */
#define NOCODE 0
#define LOWESTGIDEICODE 236

#define DOUBLECODE 243
#define LONGCODE 244
#define INTEGERCODE 245
#define BYTECODE 246
#define BLKTRANSCODE 247
#define DELIMITERCODE 248
#define ENDCODE 249
#define BEGINCODE 250
#define CLEARCODE 251
#define DEFAULTCODE 252
#define UNKNOWNCODE 253
#define EXTENDEDCODE 254
#define TERMCODE 255

/* Command Codes */
#define KBDPRESSCODE 2
#define KBDCOMBINECODE 3
#define KBDHOLDCODE 4
#define KBDLOCKCODE 5
#define KBDRELCODE 6
#define KBDEXPANSIONCODE 10

#define MOULOCKCODE 11
#define MOURELCODE 12
#define MOUCLICKCODE 13
#define MOUDOUBLECLICKCODE 14
#define MOUMOVECODE 15
#define MOUGOTOCODE 16
#define MOURESETCODE 17
#define MOUANCHORCODE   18
#define MOUEXPANSIONCODE 20

#define COMMCODE 150
#define BAUDRATECODE 151
#define GENCODE 160
#define DEBUGCODE 220

/* Model Codes */

#define IBMEXTENDEDCODE 	1
#define IBMATCODE		 	2
#define IBMPCCODE			3

#define KBDINDICATORCODE 6
#define KBDVERSIONCODE	7
#define KBDMODELCODE 	8
#define KBDDESCRIPTIONCODE 9
#define KBDUNKNOWNCODE 10

#define NOBUTTON			0
#define DEFAULTBUTTONCODE	1
#define LEFTBUTTONCODE		1
#define RIGHTBUTTONCODE		2

/* BAUDRATE CODES  */

#define BAUD300CODE	1
#define BAUD600CODE  2
#define BAUD1200CODE	3
#define BAUD2400CODE	4
#define BAUD4800CODE	5
#define BAUD9600CODE	6
#define BAUD19200CODE	7







/****************************************************************************

	The Key Code is the GIDEI standard Key Code.  The IBM Key Number is the
	number used in the IBM techincal reference of the American English
	extended 101 key keyboard.  It is included only for reference as to
	how this implementation mapped the key code to the IBM keys.

		Name			Key Code	IBM Key Number
****************************************************************************/


/* GIDEI KEY CODES */
/*************************************************************************/
/*************************************************************************/
/* Internal Key number table  */

#define	NOCODE			0
#define	NOKEY				0
#define	no_key			0
#define	lquote_key		1
#define	one_key			2
#define	two_key			3
#define	three_key		4
#define	four_key			5
#define	five_key			6
#define	six_key			7
#define	seven_key		8
#define	eight_key		9
#define	nine_key			10
#define	zero_key			11
#define	hyphen_key		12
#define	equal_key		13
#define	backspace_key	15

#define	tab_key			16
#define	q_key				17
#define	w_key				18
#define	e_key				19
#define	r_key				20
#define	t_key				21
#define	y_key				22
#define	u_key				23
#define	i_key				24
#define	o_key				25
#define	p_key				26
#define	lbracket_key	27
#define	rbracket_key	28
#define	bslash_key		29

#define	caps_key			30
#define	a_key				31
#define	s_key				32
#define	d_key				33
#define	f_key				34
#define	g_key				35
#define	h_key				36
#define	j_key				37
#define	k_key				38
#define	l_key				39
#define	semicolon_key	40
#define	rquote_key		41
#define	return_key		43

#define	lshift_key		44
#define	z_key				46
#define	x_key				47
#define	c_key				48
#define	v_key				49
#define	b_key				50
#define	n_key				51
#define	m_key				52
#define	comma_key		53
#define	period_key		54
#define	fslash_key		55
#define	rshift_key		57

#define	lcontrol_key	58
#define	lcommand_key	59
#define	lalt_key			60
#define	space_key		61
#define	ralt_key			62
#define	rcommand_key	63
#define	rcontrol_key	64

#define	insert_key		75
#define	delete_key		76
#define	left_key			79
#define	home_key			80
#define	end_key			81
#define	up_key			83
#define	down_key			84
#define	pageup_key		85
#define	pagedown_key	86
#define	right_key		89

#define	numlock_key		90
#define	kp7_key			91
#define	kp4_key			92
#define	kp1_key			93
#define	kpfslash_key	95
#define	kp8_key			96
#define	kp5_key			97
#define	kp2_key			98
#define	kp0_key			99
#define	kpstar_key		100
#define	kp9_key			101
#define	kp6_key			102
#define	kp3_key			103
#define	kpperiod_key	104
#define	kpminus_key		105
#define	kpplus_key		106
#define	kpequal_key		107
#define	kpenter_key		108

#define	escape_key		110

#define	f1_key			112
#define	f2_key			113
#define	f3_key			114
#define	f4_key			115

#define	f5_key			116
#define	f6_key			117
#define	f7_key			118
#define	f8_key			119

#define	f9_key			120
#define	f10_key			121
#define	f11_key			122
#define	f12_key			123

#define	print_key		124
#define	scroll_key		125
#define	pause_key		126
#define	reset_key		127

#define	shift_key		lshift_key
#define	control_key		lcontrol_key
#define	alt_key			lalt_key

#define	shift_Code		lshift_key
#define	control_Code	lcontrol_key
