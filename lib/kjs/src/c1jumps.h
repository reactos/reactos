/* operand halt (0) */
case 0:
  SAVE_OP (&&op_halt);
  cp += 0;
  break;

/* operand done (1) */
case 1:
  SAVE_OP (&&op_done);
  cp += 0;
  break;

/* operand nop (2) */
case 2:
  SAVE_OP (&&op_nop);
  cp += 0;
  break;

/* operand dup (3) */
case 3:
  SAVE_OP (&&op_dup);
  cp += 0;
  break;

/* operand pop (4) */
case 4:
  SAVE_OP (&&op_pop);
  cp += 0;
  break;

/* operand pop_n (5) */
case 5:
  SAVE_OP (&&op_pop_n);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand apop (6) */
case 6:
  SAVE_OP (&&op_apop);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand swap (7) */
case 7:
  SAVE_OP (&&op_swap);
  cp += 0;
  break;

/* operand roll (8) */
case 8:
  SAVE_OP (&&op_roll);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand const (9) */
case 9:
  SAVE_OP (&&op_const);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand const_null (10) */
case 10:
  SAVE_OP (&&op_const_null);
  cp += 0;
  break;

/* operand const_true (11) */
case 11:
  SAVE_OP (&&op_const_true);
  cp += 0;
  break;

/* operand const_false (12) */
case 12:
  SAVE_OP (&&op_const_false);
  cp += 0;
  break;

/* operand const_undefined (13) */
case 13:
  SAVE_OP (&&op_const_undefined);
  cp += 0;
  break;

/* operand const_i0 (14) */
case 14:
  SAVE_OP (&&op_const_i0);
  cp += 0;
  break;

/* operand const_i1 (15) */
case 15:
  SAVE_OP (&&op_const_i1);
  cp += 0;
  break;

/* operand const_i2 (16) */
case 16:
  SAVE_OP (&&op_const_i2);
  cp += 0;
  break;

/* operand const_i3 (17) */
case 17:
  SAVE_OP (&&op_const_i3);
  cp += 0;
  break;

/* operand const_i (18) */
case 18:
  SAVE_OP (&&op_const_i);
  JS_BC_READ_INT32 (cp, i);
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand load_global (19) */
case 19:
  SAVE_OP (&&op_load_global);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand store_global (20) */
case 20:
  SAVE_OP (&&op_store_global);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand load_arg (21) */
case 21:
  SAVE_OP (&&op_load_arg);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand store_arg (22) */
case 22:
  SAVE_OP (&&op_store_arg);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand load_local (23) */
case 23:
  SAVE_OP (&&op_load_local);
  JS_BC_READ_INT16 (cp, i);
  SAVE_INT16 (i);
  cp += 2;
  break;

/* operand store_local (24) */
case 24:
  SAVE_OP (&&op_store_local);
  JS_BC_READ_INT16 (cp, i);
  SAVE_INT16 (i);
  cp += 2;
  break;

/* operand load_property (25) */
case 25:
  SAVE_OP (&&op_load_property);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand store_property (26) */
case 26:
  SAVE_OP (&&op_store_property);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand load_array (27) */
case 27:
  SAVE_OP (&&op_load_array);
  cp += 0;
  break;

/* operand store_array (28) */
case 28:
  SAVE_OP (&&op_store_array);
  cp += 0;
  break;

/* operand nth (29) */
case 29:
  SAVE_OP (&&op_nth);
  cp += 0;
  break;

/* operand cmp_eq (30) */
case 30:
  SAVE_OP (&&op_cmp_eq);
  cp += 0;
  break;

/* operand cmp_ne (31) */
case 31:
  SAVE_OP (&&op_cmp_ne);
  cp += 0;
  break;

/* operand cmp_lt (32) */
case 32:
  SAVE_OP (&&op_cmp_lt);
  cp += 0;
  break;

/* operand cmp_gt (33) */
case 33:
  SAVE_OP (&&op_cmp_gt);
  cp += 0;
  break;

/* operand cmp_le (34) */
case 34:
  SAVE_OP (&&op_cmp_le);
  cp += 0;
  break;

/* operand cmp_ge (35) */
case 35:
  SAVE_OP (&&op_cmp_ge);
  cp += 0;
  break;

/* operand cmp_seq (36) */
case 36:
  SAVE_OP (&&op_cmp_seq);
  cp += 0;
  break;

/* operand cmp_sne (37) */
case 37:
  SAVE_OP (&&op_cmp_sne);
  cp += 0;
  break;

/* operand sub (38) */
case 38:
  SAVE_OP (&&op_sub);
  cp += 0;
  break;

/* operand add (39) */
case 39:
  SAVE_OP (&&op_add);
  cp += 0;
  break;

/* operand mul (40) */
case 40:
  SAVE_OP (&&op_mul);
  cp += 0;
  break;

/* operand div (41) */
case 41:
  SAVE_OP (&&op_div);
  cp += 0;
  break;

/* operand mod (42) */
case 42:
  SAVE_OP (&&op_mod);
  cp += 0;
  break;

/* operand neg (43) */
case 43:
  SAVE_OP (&&op_neg);
  cp += 0;
  break;

/* operand and (44) */
case 44:
  SAVE_OP (&&op_and);
  cp += 0;
  break;

/* operand not (45) */
case 45:
  SAVE_OP (&&op_not);
  cp += 0;
  break;

/* operand or (46) */
case 46:
  SAVE_OP (&&op_or);
  cp += 0;
  break;

/* operand xor (47) */
case 47:
  SAVE_OP (&&op_xor);
  cp += 0;
  break;

/* operand shift_left (48) */
case 48:
  SAVE_OP (&&op_shift_left);
  cp += 0;
  break;

/* operand shift_right (49) */
case 49:
  SAVE_OP (&&op_shift_right);
  cp += 0;
  break;

/* operand shift_rright (50) */
case 50:
  SAVE_OP (&&op_shift_rright);
  cp += 0;
  break;

/* operand iffalse (51) */
case 51:
  SAVE_OP (&&op_iffalse);
  JS_BC_READ_INT32 (cp, i);
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand iftrue (52) */
case 52:
  SAVE_OP (&&op_iftrue);
  JS_BC_READ_INT32 (cp, i);
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand call_method (53) */
case 53:
  SAVE_OP (&&op_call_method);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand jmp (54) */
case 54:
  SAVE_OP (&&op_jmp);
  JS_BC_READ_INT32 (cp, i);
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand jsr (55) */
case 55:
  SAVE_OP (&&op_jsr);
  cp += 0;
  break;

/* operand return (56) */
case 56:
  SAVE_OP (&&op_return);
  cp += 0;
  break;

/* operand typeof (57) */
case 57:
  SAVE_OP (&&op_typeof);
  cp += 0;
  break;

/* operand new (58) */
case 58:
  SAVE_OP (&&op_new);
  cp += 0;
  break;

/* operand delete_property (59) */
case 59:
  SAVE_OP (&&op_delete_property);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand delete_array (60) */
case 60:
  SAVE_OP (&&op_delete_array);
  cp += 0;
  break;

/* operand locals (61) */
case 61:
  SAVE_OP (&&op_locals);
  JS_BC_READ_INT16 (cp, i);
  SAVE_INT16 (i);
  cp += 2;
  break;

/* operand min_args (62) */
case 62:
  SAVE_OP (&&op_min_args);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand load_nth_arg (63) */
case 63:
  SAVE_OP (&&op_load_nth_arg);
  cp += 0;
  break;

/* operand with_push (64) */
case 64:
  SAVE_OP (&&op_with_push);
  cp += 0;
  break;

/* operand with_pop (65) */
case 65:
  SAVE_OP (&&op_with_pop);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand try_push (66) */
case 66:
  SAVE_OP (&&op_try_push);
  JS_BC_READ_INT32 (cp, i);
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand try_pop (67) */
case 67:
  SAVE_OP (&&op_try_pop);
  JS_BC_READ_INT8 (cp, i);
  SAVE_INT8 (i);
  cp += 1;
  break;

/* operand throw (68) */
case 68:
  SAVE_OP (&&op_throw);
  cp += 0;
  break;

/* operand iffalse_b (69) */
case 69:
  SAVE_OP (&&op_iffalse_b);
  JS_BC_READ_INT32 (cp, i);
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand iftrue_b (70) */
case 70:
  SAVE_OP (&&op_iftrue_b);
  JS_BC_READ_INT32 (cp, i);
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand add_1_i (71) */
case 71:
  SAVE_OP (&&op_add_1_i);
  cp += 0;
  break;

/* operand add_2_i (72) */
case 72:
  SAVE_OP (&&op_add_2_i);
  cp += 0;
  break;

/* operand load_global_w (73) */
case 73:
  SAVE_OP (&&op_load_global_w);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

/* operand jsr_w (74) */
case 74:
  SAVE_OP (&&op_jsr_w);
  JS_BC_READ_INT32 (cp, i);
  i += consts_offset;
  i = vm->consts[i].u.vsymbol;
  SAVE_INT32 (i);
  cp += 4;
  break;

