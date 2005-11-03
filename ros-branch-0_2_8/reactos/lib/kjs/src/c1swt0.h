/* operand halt (0) */
case 0:
  cp += 0;
  break;

/* operand done (1) */
case 1:
  cp += 0;
  break;

/* operand nop (2) */
case 2:
  cp += 0;
  break;

/* operand dup (3) */
case 3:
  cp += 0;
  break;

/* operand pop (4) */
case 4:
  cp += 0;
  break;

/* operand pop_n (5) */
case 5:
  cp += 1;
  break;

/* operand apop (6) */
case 6:
  cp += 1;
  break;

/* operand swap (7) */
case 7:
  cp += 0;
  break;

/* operand roll (8) */
case 8:
  cp += 1;
  break;

/* operand const (9) */
case 9:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand const_null (10) */
case 10:
  cp += 0;
  break;

/* operand const_true (11) */
case 11:
  cp += 0;
  break;

/* operand const_false (12) */
case 12:
  cp += 0;
  break;

/* operand const_undefined (13) */
case 13:
  cp += 0;
  break;

/* operand const_i0 (14) */
case 14:
  cp += 0;
  break;

/* operand const_i1 (15) */
case 15:
  cp += 0;
  break;

/* operand const_i2 (16) */
case 16:
  cp += 0;
  break;

/* operand const_i3 (17) */
case 17:
  cp += 0;
  break;

/* operand const_i (18) */
case 18:
  cp += 4;
  break;

/* operand load_global (19) */
case 19:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand store_global (20) */
case 20:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand load_arg (21) */
case 21:
  cp += 1;
  break;

/* operand store_arg (22) */
case 22:
  cp += 1;
  break;

/* operand load_local (23) */
case 23:
  cp += 2;
  break;

/* operand store_local (24) */
case 24:
  cp += 2;
  break;

/* operand load_property (25) */
case 25:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand store_property (26) */
case 26:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand load_array (27) */
case 27:
  cp += 0;
  break;

/* operand store_array (28) */
case 28:
  cp += 0;
  break;

/* operand nth (29) */
case 29:
  cp += 0;
  break;

/* operand cmp_eq (30) */
case 30:
  cp += 0;
  break;

/* operand cmp_ne (31) */
case 31:
  cp += 0;
  break;

/* operand cmp_lt (32) */
case 32:
  cp += 0;
  break;

/* operand cmp_gt (33) */
case 33:
  cp += 0;
  break;

/* operand cmp_le (34) */
case 34:
  cp += 0;
  break;

/* operand cmp_ge (35) */
case 35:
  cp += 0;
  break;

/* operand cmp_seq (36) */
case 36:
  cp += 0;
  break;

/* operand cmp_sne (37) */
case 37:
  cp += 0;
  break;

/* operand sub (38) */
case 38:
  cp += 0;
  break;

/* operand add (39) */
case 39:
  cp += 0;
  break;

/* operand mul (40) */
case 40:
  cp += 0;
  break;

/* operand div (41) */
case 41:
  cp += 0;
  break;

/* operand mod (42) */
case 42:
  cp += 0;
  break;

/* operand neg (43) */
case 43:
  cp += 0;
  break;

/* operand and (44) */
case 44:
  cp += 0;
  break;

/* operand not (45) */
case 45:
  cp += 0;
  break;

/* operand or (46) */
case 46:
  cp += 0;
  break;

/* operand xor (47) */
case 47:
  cp += 0;
  break;

/* operand shift_left (48) */
case 48:
  cp += 0;
  break;

/* operand shift_right (49) */
case 49:
  cp += 0;
  break;

/* operand shift_rright (50) */
case 50:
  cp += 0;
  break;

/* operand iffalse (51) */
case 51:
  cp += 4;
  break;

/* operand iftrue (52) */
case 52:
  cp += 4;
  break;

/* operand call_method (53) */
case 53:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand jmp (54) */
case 54:
  cp += 4;
  break;

/* operand jsr (55) */
case 55:
  cp += 0;
  break;

/* operand return (56) */
case 56:
  cp += 0;
  break;

/* operand typeof (57) */
case 57:
  cp += 0;
  break;

/* operand new (58) */
case 58:
  cp += 0;
  break;

/* operand delete_property (59) */
case 59:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand delete_array (60) */
case 60:
  cp += 0;
  break;

/* operand locals (61) */
case 61:
  cp += 2;
  break;

/* operand min_args (62) */
case 62:
  cp += 1;
  break;

/* operand load_nth_arg (63) */
case 63:
  cp += 0;
  break;

/* operand with_push (64) */
case 64:
  cp += 0;
  break;

/* operand with_pop (65) */
case 65:
  cp += 1;
  break;

/* operand try_push (66) */
case 66:
  cp += 4;
  break;

/* operand try_pop (67) */
case 67:
  cp += 1;
  break;

/* operand throw (68) */
case 68:
  cp += 0;
  break;

/* operand iffalse_b (69) */
case 69:
  cp += 4;
  break;

/* operand iftrue_b (70) */
case 70:
  cp += 4;
  break;

/* operand add_1_i (71) */
case 71:
  cp += 0;
  break;

/* operand add_2_i (72) */
case 72:
  cp += 0;
  break;

/* operand load_global_w (73) */
case 73:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

/* operand jsr_w (74) */
case 74:
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  JS_BC_WRITE_INT32 (cp, i);
  cp += 4;
  break;

