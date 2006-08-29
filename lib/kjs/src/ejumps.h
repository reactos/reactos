/* operand halt (0) */
op_halt:
  OPERAND (0);
  sprintf (buf, "VM: halt%s", JS_HOST_LINE_BREAK);

  js_iostream_write (vm->s_stderr, buf, strlen (buf));
  js_iostream_flush (vm->s_stderr);

  while (1)
    sleep (5);
  NEXT ();

/* operand done (1) */
op_done:
  OPERAND (1);
  DONE ();
  NEXT ();

/* operand nop (2) */
op_nop:
  OPERAND (2);
  /* Nothing here! */
  NEXT ();

/* operand dup (3) */
op_dup:
  OPERAND (3);
  JS_COPY (JS_SP0, JS_SP1);
  JS_PUSH ();
  NEXT ();

/* operand pop (4) */
op_pop:
  OPERAND (4);
  JS_POP ();
  NEXT ();

/* operand pop_n (5) */
op_pop_n:
  OPERAND (5);
  READ_INT8 (i);
  JS_POP_N (i);
  NEXT ();

/* operand apop (6) */
op_apop:
  OPERAND (6);
  READ_INT8 (i);
  JS_COPY (JS_SP (i + 1), JS_SP1);
  JS_POP_N (i);
  NEXT ();

/* operand swap (7) */
op_swap:
  OPERAND (7);
  JS_COPY (JS_SP0, JS_SP2);
  JS_COPY (JS_SP2, JS_SP1);
  JS_COPY (JS_SP1, JS_SP0);
  NEXT ();

/* operand roll (8) */
op_roll:
  OPERAND (8);
  READ_INT8 (i8);

  if (i8 > 1)
    {
      int j;

      for (j = 0; j < i8; j++)
	JS_COPY (JS_SP (j), JS_SP (j + 1));

      JS_COPY (JS_SP (i8), JS_SP0);
    }
  else if (i8 < -1)
    {
      i8 = -i8;

      JS_COPY (JS_SP0, JS_SP (i8));
      for (; i8 > 0; i8--)
	JS_COPY (JS_SP (i8), JS_SP (i8 - 1));
    }
  NEXT ();

/* operand const (9) */
op_const:
  OPERAND (9);
  READ_INT32 (i);
  JS_COPY (JS_SP0, JS_CONST (i));
  JS_PUSH ();
  NEXT ();

/* operand const_null (10) */
op_const_null:
  OPERAND (10);
  JS_SP0->type = JS_NULL;
  JS_PUSH ();
  NEXT ();

/* operand const_true (11) */
op_const_true:
  OPERAND (11);
  JS_SP0->type = JS_BOOLEAN;
  JS_SP0->u.vboolean = 1;
  JS_PUSH ();
  NEXT ();

/* operand const_false (12) */
op_const_false:
  OPERAND (12);
  JS_SP0->type = JS_BOOLEAN;
  JS_SP0->u.vboolean = 0;
  JS_PUSH ();
  NEXT ();

/* operand const_undefined (13) */
op_const_undefined:
  OPERAND (13);
  JS_SP0->type = JS_UNDEFINED;
  JS_PUSH ();
  NEXT ();

/* operand const_i0 (14) */
op_const_i0:
  OPERAND (14);
  JS_SP0->type = JS_INTEGER;
  JS_SP0->u.vinteger = 0;
  JS_PUSH ();
  NEXT ();

/* operand const_i1 (15) */
op_const_i1:
  OPERAND (15);
  JS_SP0->type = JS_INTEGER;
  JS_SP0->u.vinteger = 1;
  JS_PUSH ();
  NEXT ();

/* operand const_i2 (16) */
op_const_i2:
  OPERAND (16);
  JS_SP0->type = JS_INTEGER;
  JS_SP0->u.vinteger = 2;
  JS_PUSH ();
  NEXT ();

/* operand const_i3 (17) */
op_const_i3:
  OPERAND (17);
  JS_SP0->type = JS_INTEGER;
  JS_SP0->u.vinteger = 3;
  JS_PUSH ();
  NEXT ();

/* operand const_i (18) */
op_const_i:
  OPERAND (18);
  READ_INT32 (i);
  JS_SP0->type = JS_INTEGER;
  JS_SP0->u.vinteger = i;
  JS_PUSH ();
  NEXT ();

/* operand load_global (19) */
op_load_global:
  OPERAND (19);
  READ_INT32 (j);

  /* Use the global value only. */
  JS_COPY (JS_SP0, JS_GLOBAL (j));
  JS_PUSH ();

  if (vm->warn_undef && JS_SP1->type == JS_UNDEFINED)
    {
      sprintf (buf, "VM: warning: using undefined global `%s'%s",
	       js_vm_symname (vm, j), JS_HOST_LINE_BREAK);
      js_iostream_write (vm->s_stderr, buf, strlen (buf));
    }
  NEXT ();

/* operand store_global (20) */
op_store_global:
  OPERAND (20);
  READ_INT32 (i);

  /* Operand store_global do not check the with-chain. */
  /* WITHCHAIN */

  /* Set the global value. */
  JS_COPY (JS_GLOBAL (i), JS_SP1);
  JS_POP ();
  NEXT ();

/* operand load_arg (21) */
op_load_arg:
  OPERAND (21);
  READ_INT8 (i);
  JS_COPY (JS_SP0, JS_ARG (i));
  JS_PUSH ();
  NEXT ();

/* operand store_arg (22) */
op_store_arg:
  OPERAND (22);
  READ_INT8 (i);
  JS_COPY (JS_ARG (i), JS_SP1);
  JS_POP ();
  NEXT ();

/* operand load_local (23) */
op_load_local:
  OPERAND (23);
  READ_INT16 (i);
  JS_COPY (JS_SP0, JS_LOCAL (i));
  JS_PUSH ();
  NEXT ();

/* operand store_local (24) */
op_store_local:
  OPERAND (24);
  READ_INT16 (i);
  JS_COPY (JS_LOCAL (i), JS_SP1);
  JS_POP ();
  NEXT ();

/* operand load_property (25) */
op_load_property:
  OPERAND (25);
  /* Fetch the property symbol. */
  READ_INT32 (j);

  if (JS_SP1->type == JS_BUILTIN)
    {
      JS_SAVE_REGS ();
      if (JS_SP1->u.vbuiltin->info->property_proc)
	{
	  if ((*JS_SP1->u.vbuiltin->info->property_proc) (
					vm,
					JS_SP1->u.vbuiltin->info,
					JS_SP1->u.vbuiltin->instance_context,
					j, 0, &builtin_result)
	      == JS_PROPERTY_UNKNOWN)
	    {
	      if (j == vm->syms.s_prototype)
		{
		  /* Looking up the prototype. */

		  builtin_result.type = JS_OBJECT;
		  if (JS_SP1->u.vbuiltin->prototype)
		    /* This is an instance. */
		    builtin_result.u.vobject = JS_SP1->u.vbuiltin->prototype;
		  else
		    /* This is a class. */
		    builtin_result.u.vobject
		      = JS_SP1->u.vbuiltin->info->prototype;
		}
	      else
		{
		  /* Looking up stuffs from the prototype. */

		  if (JS_SP1->u.vbuiltin->prototype)
		    /* An instance. */
		    js_vm_object_load_property (vm,
						JS_SP1->u.vbuiltin->prototype,
						j, &builtin_result);
		  else
		    /* A class. */
		    js_vm_object_load_property (
					vm,
					JS_SP1->u.vbuiltin->info->prototype,
					j, &builtin_result);
		}
	    }
	  JS_COPY (JS_SP1, &builtin_result);
	}
      else
	ERROR ("illegal builtin object for load_property");
    }
  else if (JS_SP1->type == JS_OBJECT)
    {
      js_vm_object_load_property (vm, JS_SP1->u.vobject, j, JS_SP1);
    }
  else if (vm->prim[JS_SP1->type])
    {
      /* The primitive language types. */
      JS_SAVE_REGS ();
      if ((*vm->prim[JS_SP1->type]->property_proc) (vm, vm->prim[JS_SP1->type],
						    JS_SP1, j, 0,
						    &builtin_result)
	  == JS_PROPERTY_UNKNOWN)
	{
	  if (j == vm->syms.s_prototype)
	    {
	      /* Looking up the prototype. */
	      switch (JS_SP1->type)
		{
		case JS_STRING:
		  if (JS_SP1->u.vstring->prototype)
		    {
		      builtin_result.type = JS_OBJECT;
		      builtin_result.u.vobject = JS_SP1->u.vstring->prototype;
		    }
		  else
		    /* No prototype yet. */
		    builtin_result.type = JS_NULL;
		  break;

		case JS_ARRAY:
		  if (JS_SP1->u.varray->prototype)
		    {
		      builtin_result.type = JS_OBJECT;
		      builtin_result.u.vobject = JS_SP1->u.varray->prototype;
		    }
		  else
		    /* No prototype yet. */
		    builtin_result.type = JS_NULL;
		  break;

		case JS_FUNC:
		  if (JS_SP1->u.vfunction->prototype)
		    {
		      builtin_result.type = JS_OBJECT;
		      builtin_result.u.vobject
			= JS_SP1->u.vfunction->prototype;
		    }
		  else
		    /* No prototype yet. */
		    builtin_result.type = JS_NULL;
		  break;

		default:
		  /* The rest do not have prototype. */
		  builtin_result.type = JS_NULL;
		  break;
		}
	    }
	  else
	    {
	      /* Looking up stuffs from the prototype. */
	      switch (JS_SP1->type)
		{
		case JS_STRING:
		  if (JS_SP1->u.vstring->prototype)
		    js_vm_object_load_property (vm,
						JS_SP1->u.vstring->prototype,
						j, &builtin_result);
		  else
		    /* Take it from the class' prototype */
		    goto _op_load_property_try_proto;
		  break;

		case JS_ARRAY:
		  if (JS_SP1->u.varray->prototype)
		    js_vm_object_load_property (vm,
						JS_SP1->u.varray->prototype,
						j, &builtin_result);
		  else
		    /* Take it from the class' prototype */
		    goto _op_load_property_try_proto;
		  break;

		case JS_FUNC:
		  if (JS_SP1->u.vfunction->prototype)
		    js_vm_object_load_property (vm,
						JS_SP1->u.vfunction->prototype,
						j, &builtin_result);
		  else
		    /* Take it from the class' prototype */
		    goto _op_load_property_try_proto;
		  break;

		default:
		  /*
		   * The rest do not have instance prototypes; use the
		   * class prototypes.
		   */
		_op_load_property_try_proto:
		  js_vm_object_load_property (
					vm,
					vm->prim[JS_SP1->type]->prototype, j,
					&builtin_result);
		  break;
		}
	    }
	}

      JS_COPY (JS_SP1, &builtin_result);
    }
  else
    ERROR ("illegal object for load_property");
  NEXT ();

/* operand store_property (26) */
op_store_property:
  OPERAND (26);
  /* Fetch the property symbol. */
  READ_INT32 (j);

  if (JS_SP1->type == JS_BUILTIN)
    {
      JS_SAVE_REGS ();
      if (JS_SP1->u.vbuiltin->info->property_proc)
	{
	  if ((*JS_SP1->u.vbuiltin->info->property_proc) (
					vm,
					JS_SP1->u.vbuiltin->info,
					JS_SP1->u.vbuiltin->instance_context,
					j, 1, JS_SP2)
	      == JS_PROPERTY_UNKNOWN)
	    {
	      if (j == vm->syms.s_prototype)
		{
		  /* Setting the prototype. */
		  if (JS_SP2->type != JS_OBJECT)
		    ERROR ("illegal value for set_property");

		  if (JS_SP1->u.vbuiltin->prototype)
		    /* Setting the instance's prototype. */
		    JS_SP1->u.vbuiltin->prototype = JS_SP2->u.vobject;
		  else
		    /* Setting the class' prototype. */
		    JS_SP1->u.vbuiltin->info->prototype = JS_SP2->u.vobject;
		}
	      else
		{
		  /* Setting stuff to the prototype. */
		  if (JS_SP1->u.vbuiltin->prototype)
		    /* An instance. */
		    js_vm_object_store_property (vm,
						 JS_SP1->u.vbuiltin->prototype,
						 j, JS_SP2);
		  else
		    /* A class. */
		    js_vm_object_store_property (
					vm,
					JS_SP1->u.vbuiltin->info->prototype,
					j, JS_SP2);
		}
	    }
	}
      else
	ERROR ("illegal builtin object for store_property");

      JS_POP ();
      JS_POP ();
    }
  else if (JS_SP1->type == JS_OBJECT)
    {
      js_vm_object_store_property (vm, JS_SP1->u.vobject, j, JS_SP2);
      JS_POP ();
      JS_POP ();
    }
  else if (vm->prim[JS_SP1->type])
    {
      /* The primitive language types. */
      JS_SAVE_REGS ();
      if ((*vm->prim[JS_SP1->type]->property_proc) (vm, vm->prim[JS_SP1->type],
						    JS_SP1, j, 1, JS_SP2)
	  == JS_PROPERTY_UNKNOWN)
	{
	  if (j == vm->syms.s_prototype)
	    {
	      /* Setting the prototype. */
	      if (JS_SP2->type != JS_OBJECT)
		ERROR ("illegal value for set_property");

	      switch (JS_SP1->type)
		{
		case JS_STRING:
		  JS_SP1->u.vstring->prototype = JS_SP2->u.vobject;
		  break;

		case JS_ARRAY:
		  JS_SP1->u.varray->prototype = JS_SP2->u.vobject;
		  break;

		case JS_FUNC:
		  JS_SP1->u.vfunction->prototype = JS_SP2->u.vobject;
		  break;

		default:
		  ERROR ("illegal object for set_property");
		  break;
		}
	    }
	  else
	    {
	      JSNode prototype;

	      /* Setting to the prototype.  We create them on demand. */
	      switch (JS_SP1->type)
		{
		case JS_STRING:
		  if (JS_SP1->u.vstring->prototype == NULL)
		    {
		      prototype.type = JS_OBJECT;

		      /* Create the prototype and set its __proto__. */
		      JS_SP1->u.vstring->prototype = js_vm_object_new (vm);
		      prototype.u.vobject = vm->prim[JS_OBJECT]->prototype;
		      js_vm_object_store_property (
					vm,
					JS_SP1->u.vstring->prototype,
					vm->syms.s___proto__,
					&prototype);
		    }
		  js_vm_object_store_property (vm,
					       JS_SP1->u.vstring->prototype,
					       j, JS_SP2);
		  break;

		case JS_ARRAY:
		  if (JS_SP1->u.varray->prototype == NULL)
		    {
		      prototype.type = JS_OBJECT;

		      /* Create the prototype and set its __proto__. */
		      JS_SP1->u.varray->prototype = js_vm_object_new (vm);
		      prototype.u.vobject = vm->prim[JS_OBJECT]->prototype;
		      js_vm_object_store_property (
					vm,
					JS_SP1->u.varray->prototype,
					vm->syms.s___proto__,
					&prototype);
		    }
		  js_vm_object_store_property (vm,
					       JS_SP1->u.varray->prototype,
					       j, JS_SP2);
		  break;

		case JS_FUNC:
		  if (JS_SP1->u.vfunction->prototype == NULL)
		    {
		      prototype.type = JS_OBJECT;

		      /* Create the prototype and set its __proto__. */
		      JS_SP1->u.vfunction->prototype = js_vm_object_new (vm);
		      prototype.u.vobject = vm->prim[JS_OBJECT]->prototype;
		      js_vm_object_store_property (
					vm,
					JS_SP1->u.vfunction->prototype,
					vm->syms.s___proto__,
					&prototype);
		    }
		  js_vm_object_store_property (vm,
					       JS_SP1->u.vfunction->prototype,
					       j, JS_SP2);
		  break;

		default:
		  ERROR ("illegal object for set_property");
		  break;
		}
	    }
	}
      JS_POP ();
      JS_POP ();
    }
  else
    ERROR ("illegal object for store_property");

  JS_MAYBE_GC ();
  NEXT ();

/* operand load_array (27) */
op_load_array:
  OPERAND (27);
  if (JS_SP2->type == JS_BUILTIN)
    {
      if (JS_SP1->type == JS_INTEGER)
	{
	  ERROR ("integer indexes not implemented yet for BUILTIN in load_array");
	}
      else if (JS_SP1->type == JS_STRING)
	{
	  /* Intern the string. */
	  j = js_vm_intern_with_len (vm, JS_SP1->u.vstring->data,
				     JS_SP1->u.vstring->len);

	  /* The code below must be in sync with operand `load_property'. */
	  JS_SAVE_REGS ();

	  if (JS_SP2->u.vbuiltin->info->property_proc)
	    {
	      if ((*JS_SP2->u.vbuiltin->info->property_proc) (
					vm,
					JS_SP2->u.vbuiltin->info,
					JS_SP2->u.vbuiltin->instance_context,
					j, 0, &builtin_result)
		  == JS_PROPERTY_UNKNOWN)
		{
		  if (j == vm->syms.s_prototype)
		    {
		      /* Looking up the prototype. */

		      builtin_result.type = JS_OBJECT;
		      if (JS_SP2->u.vbuiltin->prototype)
			/* This is an instance. */
			builtin_result.u.vobject
			  = JS_SP2->u.vbuiltin->prototype;
		      else
			/* This is a class. */
			builtin_result.u.vobject
			  = JS_SP2->u.vbuiltin->info->prototype;
		    }
		  else
		    {
		      /* Looking up stuffs from the prototype. */

		      if (JS_SP2->u.vbuiltin->prototype)
			/* An instance. */
			js_vm_object_load_property (
						vm,
						JS_SP2->u.vbuiltin->prototype,
						j, &builtin_result);
		      else
			/* A class. */
			js_vm_object_load_property (
					vm,
					JS_SP2->u.vbuiltin->info->prototype,
					j, &builtin_result);
		    }
		}
	      JS_COPY (JS_SP2, &builtin_result);
	      JS_POP ();
	    }
	  else
	    ERROR ("illegal builtin object for load_array");
	}
      else
	{
	  sprintf (buf, "illegal array index in load_array (%d)",
		   JS_SP1->type);
	  ERROR (buf);
	}
    }
  else if (JS_SP2->type == JS_OBJECT)
    {
      js_vm_object_load_array (vm, JS_SP2->u.vobject, JS_SP1, JS_SP2);
      JS_POP ();
    }
  else if (JS_SP2->type == JS_ARRAY)
    {
      if (JS_SP1->type == JS_INTEGER)
	{
	  if (JS_SP1->u.vinteger < 0
	      || JS_SP1->u.vinteger >= JS_SP2->u.varray->length)
	    JS_SP2->type = JS_UNDEFINED;
	  else
	    {
	      JSNode *n = &JS_SP2->u.varray->data[JS_SP1->u.vinteger];
	      JS_COPY (JS_SP2, n);
	    }
	  JS_POP ();
	}
      else
	{
	  sprintf (buf, "illegal array index in load_array (%d)",
		   JS_SP1->type);
	  ERROR (buf);
	}
    }
  else if (JS_SP2->type == JS_STRING)
    {
      if (JS_SP1->type == JS_INTEGER)
	{
	  int ch;

	  if (JS_SP1->u.vinteger < 0
	      || JS_SP1->u.vinteger >= JS_SP2->u.vstring->len)
	    ERROR ("string index out of range in load_array");

	  ch = JS_SP2->u.vstring->data[JS_SP1->u.vinteger];
	  JS_SP2->type = JS_INTEGER;
	  JS_SP2->u.vinteger = ch;

	  JS_POP ();
	}
      else
	ERROR ("illegal string index in load_array");
    }
  else
    ERROR ("illegal object for load_array");
  NEXT ();

/* operand store_array (28) */
op_store_array:
  OPERAND (28);
  if (JS_SP2->type == JS_BUILTIN)
    {
      if (JS_SP1->type == JS_INTEGER)
	{
	  ERROR ("integer index not implemented yet for BUILTIN in store_array");
	}
      else if (JS_SP1->type == JS_STRING)
	{
	  /* Intern the string. */
	  j = js_vm_intern_with_len (vm, JS_SP1->u.vstring->data,
				     JS_SP1->u.vstring->len);

	  /* The code below msut be in sync with operand `store_property'. */
	  JS_SAVE_REGS ();
	  if (JS_SP2->u.vbuiltin->info->property_proc)
	    {
	      if ((*JS_SP2->u.vbuiltin->info->property_proc) (
					vm,
					JS_SP2->u.vbuiltin->info,
					JS_SP2->u.vbuiltin->instance_context,
					j, 1, JS_SP (3))
		  == JS_PROPERTY_UNKNOWN)
		{
		  if (j == vm->syms.s_prototype)
		    {
		      /* Setting the prototype. */
		      if (JS_SP (3)->type != JS_OBJECT)
			ERROR ("illegal value for prototype");

		      if (JS_SP2->u.vbuiltin->prototype)
			/* Setting the instance's prototype. */
			JS_SP2->u.vbuiltin->prototype = JS_SP (3)->u.vobject;
		      else
			/* Setting the class' prototype. */
			JS_SP2->u.vbuiltin->info->prototype
			  = JS_SP (3)->u.vobject;
		    }
		  else
		    {
		      /* Setting stuff to the prototype. */
		      if (JS_SP2->u.vbuiltin->prototype)
			/* An instance. */
			js_vm_object_store_property (
						vm,
						JS_SP2->u.vbuiltin->prototype,
						j, JS_SP (3));
		      else
			/* A class. */
			js_vm_object_store_property (
					vm,
					JS_SP2->u.vbuiltin->info->prototype,
					j, JS_SP (3));
		    }
		}
	    }
	  else
	    ERROR ("illegal builtin object for store_array");

	  JS_POP_N (3);
	}
      else
	ERROR ("illegal array index in store_array");
    }
  else if (JS_SP2->type == JS_OBJECT)
    {
      js_vm_object_store_array (vm, JS_SP2->u.vobject, JS_SP1, JS_SP (3));
      JS_POP_N (3);
    }
  else if (JS_SP2->type == JS_ARRAY)
    {
      if (JS_SP1->type == JS_INTEGER)
	{
	  if (JS_SP1->u.vinteger < 0)
	    ERROR ("negative array index in store_array");
	  if (JS_SP1->u.vinteger >= JS_SP2->u.varray->length)
	    js_vm_expand_array (vm, JS_SP2, JS_SP1->u.vinteger + 1);

	  JS_COPY (&JS_SP2->u.varray->data[JS_SP1->u.vinteger], JS_SP (3));
	  JS_POP_N (3);
	}
      else
	ERROR ("illegal array index in store_array");
    }
  else if (JS_SP2->type == JS_STRING)
    {
      if (JS_SP1->type == JS_INTEGER)
	{
	  if (JS_SP2->u.vstring->staticp)
	    ERROR ("static string in store_array");

	  if (JS_SP1->u.vinteger < 0)
	    ERROR ("negative string index in store_array");

	  if (JS_SP (3)->type != JS_INTEGER)
	    ERROR ("non-integer value to store into string in store_array");

	  if (JS_SP1->u.vinteger >= JS_SP2->u.vstring->len)
	    {
	      /* Expand the string. */
	      JS_SP2->u.vstring->data = js_vm_realloc (vm,
						       JS_SP2->u.vstring->data,
						       JS_SP1->u.vinteger + 1);
	      /* Fill the gap with ' '. */
	      for (; JS_SP2->u.vstring->len <= JS_SP1->u.vinteger;)
		JS_SP2->u.vstring->data[JS_SP2->u.vstring->len++] = ' ';
	    }

	  JS_SP2->u.vstring->data[JS_SP1->u.vinteger]
	    = (unsigned char) JS_SP (3)->u.vinteger;
	  JS_POP_N (3);
	}
      else
	ERROR ("illegal string index in store_array");
    }
  else
    ERROR ("illegal object for store_array");

  JS_MAYBE_GC ();
  NEXT ();

/* operand nth (29) */
op_nth:
  OPERAND (29);
  if (JS_SP2->type == JS_STRING)
    {
      if (JS_SP1->u.vinteger < 0
	  || JS_SP1->u.vinteger >= JS_SP2->u.vstring->len)
	{
	  JS_SP2->type = JS_UNDEFINED;

	  JS_SP1->type = JS_BOOLEAN;
	  JS_SP1->u.vboolean = 0;
	}
      else
	{
	  JS_SP2->type = JS_INTEGER;
	  JS_SP2->u.vinteger = JS_SP2->u.vstring->data[JS_SP1->u.vinteger];

	  JS_SP1->type = JS_BOOLEAN;
	  JS_SP1->u.vboolean = 1;
	}
    }
  else if (JS_SP2->type == JS_ARRAY)
    {
      if (JS_SP1->u.vinteger < 0
	  || JS_SP1->u.vinteger >= JS_SP2->u.varray->length)
	{
	  JS_SP2->type = JS_UNDEFINED;

	  JS_SP1->type = JS_BOOLEAN;
	  JS_SP1->u.vboolean = 0;
	}
      else
	{
	  JSNode *n = &JS_SP2->u.varray->data[JS_SP1->u.vinteger];
	  JS_COPY (JS_SP2, n);

	  JS_SP1->type = JS_BOOLEAN;
	  JS_SP1->u.vboolean = 1;
	}
    }
  else if (JS_SP2->type == JS_OBJECT)
    {
      i = js_vm_object_nth (vm, JS_SP2->u.vobject, JS_SP1->u.vinteger, JS_SP2);
      JS_SP1->type = JS_BOOLEAN;
      JS_SP1->u.vboolean = i;
    }
  else
    ERROR ("illegal object for nth");
  NEXT ();

/* operand cmp_eq (30) */
op_cmp_eq:
  OPERAND (30);
  JS_OPERAND_CMP_EQ (==, 1);
  NEXT ();

/* operand cmp_ne (31) */
op_cmp_ne:
  OPERAND (31);
  JS_OPERAND_CMP_EQ (!=, 0);
  NEXT ();

/* operand cmp_lt (32) */
op_cmp_lt:
  OPERAND (32);
  JS_OPERAND_CMP_REL (<);
  NEXT ();

/* operand cmp_gt (33) */
op_cmp_gt:
  OPERAND (33);
  JS_OPERAND_CMP_REL (>);
  NEXT ();

/* operand cmp_le (34) */
op_cmp_le:
  OPERAND (34);
  JS_OPERAND_CMP_REL (<=);
  NEXT ();

/* operand cmp_ge (35) */
op_cmp_ge:
  OPERAND (35);
  JS_OPERAND_CMP_REL (>=);
  NEXT ();

/* operand cmp_seq (36) */
op_cmp_seq:
  OPERAND (36);
  JS_OPERAND_CMP_SEQ (==, 1);
  NEXT ();

/* operand cmp_sne (37) */
op_cmp_sne:
  OPERAND (37);
  JS_OPERAND_CMP_SEQ (!=, 0);
  NEXT ();

/* operand sub (38) */
op_sub:
  OPERAND (38);
  if (JS_SP2->type == JS_INTEGER && JS_SP1->type == JS_INTEGER)
    {
      JS_SP2->u.vinteger -= JS_SP1->u.vinteger;
    }
  else
    {
      JSNode l_cvt, r_cvt;
      JSNode *l, *r;

      if (JS_IS_NUMBER (JS_SP2))
	l = JS_SP2;
      else
	{
	  js_vm_to_number (vm, JS_SP2, &l_cvt);
	  l = &l_cvt;
	}

      if (JS_IS_NUMBER (JS_SP1))
	r = JS_SP1;
      else
	{
	  js_vm_to_number (vm, JS_SP1, &r_cvt);
	  r = &r_cvt;
	}

      if (l->type == JS_NAN || r->type == JS_NAN)
	JS_SP2->type = JS_NAN;
      else if (l->type == JS_INTEGER)
	{
	  if (r->type == JS_INTEGER)
	    {
	      JS_SP2->type = JS_INTEGER;
	      JS_SP2->u.vinteger = l->u.vinteger - r->u.vinteger;
	    }
	  else
	    {
	      JS_SP2->type = JS_FLOAT;
	      JS_SP2->u.vfloat = (double) l->u.vinteger - r->u.vfloat;
	    }
	}
      else
	{
	  if (r->type == JS_INTEGER)
	    {
	      JS_SP2->type = JS_FLOAT;
	      JS_SP2->u.vfloat = l->u.vfloat - (double) r->u.vinteger;
	    }
	  else
	    {
	      JS_SP2->type = JS_FLOAT;
	      JS_SP2->u.vfloat = l->u.vfloat - r->u.vfloat;
	    }
	}
    }

  JS_POP ();
  NEXT ();

/* operand add (39) */
op_add:
  OPERAND (39);
  if (JS_SP2->type == JS_STRING || JS_SP1->type == JS_STRING)
    {
      unsigned char *d2, *d1, *ndata;
      unsigned int d2_len, d1_len, nlen;
      JSNode cvt;

      if (JS_SP2->type == JS_STRING)
	{
	  d2 = JS_SP2->u.vstring->data;
	  d2_len = JS_SP2->u.vstring->len;
	}
      else
	{
	  js_vm_to_string (vm, JS_SP2, &cvt);
	  d2 = cvt.u.vstring->data;
	  d2_len = cvt.u.vstring->len;
	}
      if (JS_SP1->type == JS_STRING)
	{
	  d1 = JS_SP1->u.vstring->data;
	  d1_len = JS_SP1->u.vstring->len;
	}
      else
	{
	  js_vm_to_string (vm, JS_SP1, &cvt);
	  d1 = cvt.u.vstring->data;
	  d1_len = cvt.u.vstring->len;
	}

      nlen = d2_len + d1_len;
      ndata = js_vm_alloc (vm, nlen);
      memcpy (ndata, d2, d2_len);
      memcpy (ndata + d2_len, d1, d1_len);

      js_vm_make_static_string (vm, JS_SP2, ndata, nlen);
      JS_SP2->u.vstring->staticp = 0;
      JS_POP ();
      JS_MAYBE_GC ();
    }
  else if (JS_SP2->type == JS_INTEGER && JS_SP1->type == JS_INTEGER)
    {
      JS_SP2->u.vinteger += JS_SP1->u.vinteger;
      JS_POP ();
    }
  else
    {
      JSNode l_cvt, r_cvt;
      JSNode *l, *r;

      if (JS_IS_NUMBER (JS_SP2))
	l = JS_SP2;
      else
	{
	  js_vm_to_number (vm, JS_SP2, &l_cvt);
	  l = &l_cvt;
	}

      if (JS_IS_NUMBER (JS_SP1))
	r = JS_SP1;
      else
	{
	  js_vm_to_number (vm, JS_SP1, &r_cvt);
	  r = &r_cvt;
	}

      if (l->type == JS_NAN || r->type == JS_NAN)
	JS_SP2->type = JS_NAN;
      else if (l->type == JS_INTEGER)
	{
	  if (r->type == JS_INTEGER)
	    {
	      JS_SP2->type = JS_INTEGER;
	      JS_SP2->u.vinteger = l->u.vinteger + r->u.vinteger;
	    }
	  else
	    {
	      JS_SP2->type = JS_FLOAT;
	      JS_SP2->u.vfloat = (double) l->u.vinteger + r->u.vfloat;
	    }
	}
      else
	{
	  if (r->type == JS_INTEGER)
	    {
	      JS_SP2->type = JS_FLOAT;
	      JS_SP2->u.vfloat = l->u.vfloat + (double) r->u.vinteger;
	    }
	  else
	    {
	      JS_SP2->type = JS_FLOAT;
	      JS_SP2->u.vfloat = l->u.vfloat + r->u.vfloat;
	    }
	}

      JS_POP ();
    }
  NEXT ();

/* operand mul (40) */
op_mul:
  OPERAND (40);
  if (JS_SP2->type == JS_INTEGER && JS_SP1->type == JS_INTEGER)
    {
      JS_SP2->u.vinteger *= JS_SP1->u.vinteger;
    }
  else
    {
      JSNode l_cvt, r_cvt;
      JSNode *l, *r;

      if (JS_IS_NUMBER (JS_SP2))
	l = JS_SP2;
      else
	{
	  js_vm_to_number (vm, JS_SP2, &l_cvt);
	  l = &l_cvt;
	}

      if (JS_IS_NUMBER (JS_SP1))
	r = JS_SP1;
      else
	{
	  js_vm_to_number (vm, JS_SP1, &r_cvt);
	  r = &r_cvt;
	}

      if (l->type == JS_NAN || r->type == JS_NAN)
	JS_SP2->type = JS_NAN;
      else if (l->type == JS_INTEGER)
	{
	  if (r->type == JS_INTEGER)
	    {
	      JS_SP2->type = JS_INTEGER;
	      JS_SP2->u.vinteger = l->u.vinteger * r->u.vinteger;
	    }
	  else
	    {
	      if (l->u.vinteger == 0
		  && (JS_IS_POSITIVE_INFINITY (r)
		      || JS_IS_NEGATIVE_INFINITY (r)))
		JS_SP2->type = JS_NAN;
	      else
		{
		  JS_SP2->type = JS_FLOAT;
		  JS_SP2->u.vfloat = (double) l->u.vinteger * r->u.vfloat;
		}
	    }
	}
      else
	{
	  if ((JS_IS_POSITIVE_INFINITY (l) || JS_IS_NEGATIVE_INFINITY (l))
	      && ((r->type == JS_INTEGER && r->u.vinteger == 0)
		  || (r->type == JS_FLOAT && r->u.vfloat == 0.0)))
	    JS_SP2->type = JS_NAN;
	  else
	    {
	      JS_SP2->type = JS_FLOAT;

	      if (r->type == JS_INTEGER)
		JS_SP2->u.vfloat = l->u.vfloat * (double) r->u.vinteger;
	      else
		JS_SP2->u.vfloat = l->u.vfloat * r->u.vfloat;
	    }
	}
    }

  JS_POP ();
  NEXT ();

/* operand div (41) */
op_div:
  OPERAND (41);
  {
    int nan = 0;
    double l, r;
    int l_inf = 0;
    int r_inf = 0;
    JSNode *n;
    JSNode cvt;

    /* Convert divident to float. */
    if (JS_IS_NUMBER (JS_SP2))
      n = JS_SP2;
    else
      {
	js_vm_to_number (vm, JS_SP2, &cvt);
	n = &cvt;
      }

    switch (n->type)
      {
      case JS_INTEGER:
	l = (double) n->u.vinteger;
	break;

      case JS_FLOAT:
	l = n->u.vfloat;
	if (JS_IS_POSITIVE_INFINITY (n) || JS_IS_NEGATIVE_INFINITY (n))
	  l_inf = 1;
	break;

      case JS_NAN:
      default:
	nan = 1;
	break;
      }

    /* Convert divisor to float. */
    if (JS_IS_NUMBER (JS_SP1))
      n = JS_SP1;
    else
      {
	js_vm_to_number (vm, JS_SP2, &cvt);
	n = &cvt;
      }

    switch (n->type)
      {
      case JS_INTEGER:
	r = (double) n->u.vinteger;
	break;

      case JS_FLOAT:
	r = n->u.vfloat;
	if (JS_IS_POSITIVE_INFINITY (n) || JS_IS_NEGATIVE_INFINITY (n))
	  r_inf = 1;
	break;

      case JS_NAN:
      default:
	nan = 1;
	break;
      }

    /* Do the division. */
    JS_POP ();
    if (nan || (l_inf && r_inf))
      JS_SP1->type = JS_NAN;
    else
      {
	if (l_inf && r == 0.0)
	  {
	    /* <l> is already an infinity. */
	    JS_SP1->type = JS_FLOAT;
	    JS_SP1->u.vfloat = l;
	  }
	else if (l == 0.0 && r == 0.0)
	  JS_SP1->type = JS_NAN;
	else
	  {
	    JS_SP1->type = JS_FLOAT;
	    JS_SP1->u.vfloat = l / r;
	  }
      }
  }
  NEXT ();

/* operand mod (42) */
op_mod:
  OPERAND (42);
  if (JS_SP2->type == JS_INTEGER && JS_SP1->type == JS_INTEGER)
    {
      if (JS_SP1->u.vinteger == 0)
	JS_SP2->type = JS_NAN;
      else
	JS_SP2->u.vinteger %= JS_SP1->u.vinteger;
    }
  else
    {
      JSNode l_cvt, r_cvt;
      JSNode *l, *r;

      if (JS_IS_NUMBER (JS_SP2))
	l = JS_SP2;
      else
	{
	  js_vm_to_number (vm, JS_SP2, &l_cvt);
	  l = &l_cvt;
	}

      if (JS_IS_NUMBER (JS_SP1))
	r = JS_SP1;
      else
	{
	  js_vm_to_number (vm, JS_SP1, &r_cvt);
	  r = &r_cvt;
	}

      if (l->type == JS_NAN || r->type == JS_NAN)
	JS_SP2->type = JS_NAN;
      else if (JS_IS_POSITIVE_INFINITY (l)
	       || JS_IS_NEGATIVE_INFINITY (l)
	       || ((r->type == JS_INTEGER && r->u.vinteger == 0)
		   || (r->type == JS_FLOAT && r->u.vfloat == 0.0)))
	JS_SP2->type = JS_NAN;
      else if (JS_IS_POSITIVE_INFINITY (r)
	       || JS_IS_NEGATIVE_INFINITY (r))
	JS_COPY (JS_SP2, l);
      else if ((l->type == JS_INTEGER && l->u.vinteger == 0)
	       || (l->type == JS_FLOAT && l->u.vfloat == 0.0))
	JS_COPY (JS_SP2, l);
      else
	{
	  if (l->type == JS_INTEGER && r->type == JS_INTEGER)
	    {
	      JS_SP2->type = JS_INTEGER;
	      JS_SP2->u.vinteger = l->u.vinteger % r->u.vinteger;
	    }
	  else
	    {
	      double ld, rd;
	      int full;

	      if (l->type == JS_INTEGER)
		ld = (double) l->u.vinteger;
	      else
		ld = l->u.vfloat;

	      if (r->type == JS_INTEGER)
		rd = (double) r->u.vinteger;
	      else
		rd = r->u.vfloat;

	      full = ld / rd;

	      JS_SP2->type = JS_FLOAT;
	      JS_SP2->u.vfloat = ld - (full * rd);
	    }
	}
    }

  JS_POP ();
  NEXT ();

/* operand neg (43) */
op_neg:
  OPERAND (43);
  if (JS_SP1->type == JS_INTEGER)
    JS_SP1->u.vinteger = -JS_SP1->u.vinteger;
  else if (JS_SP1->type == JS_FLOAT)
    JS_SP1->u.vfloat = -JS_SP1->u.vfloat;
  else if (JS_SP1->type == JS_NAN)
    ;
  else
    {
      JSNode cvt;

      js_vm_to_number (vm, JS_SP1, &cvt);

      JS_SP1->type = cvt.type;
      switch (cvt.type)
	{
	case JS_INTEGER:
	  JS_SP1->u.vinteger = -cvt.u.vinteger;
	  break;

	case JS_FLOAT:
	  JS_SP1->u.vfloat = -cvt.u.vfloat;
	  break;

	case JS_NAN:
	default:
	  /* Nothing here. */
	  break;
	}
    }
  NEXT ();

/* operand and (44) */
op_and:
  OPERAND (44);
  JS_OPERAND_BINARY (&);
  NEXT ();

/* operand not (45) */
op_not:
  OPERAND (45);
  JS_SP1->u.vboolean = JS_IS_FALSE (JS_SP1);
  JS_SP1->type = JS_BOOLEAN;
  NEXT ();

/* operand or (46) */
op_or:
  OPERAND (46);
  JS_OPERAND_BINARY (|);
  NEXT ();

/* operand xor (47) */
op_xor:
  OPERAND (47);
  JS_OPERAND_BINARY (^);
  NEXT ();

/* operand shift_left (48) */
op_shift_left:
  OPERAND (48);
  if (JS_SP2->type == JS_INTEGER && JS_SP1->type == JS_INTEGER)
    {
      JS_SP2->u.vinteger = ((JSInt32) JS_SP2->u.vinteger
			    << (JSUInt32) JS_SP1->u.vinteger);
      JS_POP ();
    }
  else
    {
      JSInt32 l;
      JSUInt32 r;

      l = js_vm_to_int32 (vm, JS_SP2);
      r = (JSUInt32) js_vm_to_int32 (vm, JS_SP1);

      JS_SP2->u.vinteger = l << r;
      JS_SP2->type = JS_INTEGER;
      JS_POP ();
    }
  NEXT ();

/* operand shift_right (49) */
op_shift_right:
  OPERAND (49);
  if (JS_SP2->type == JS_INTEGER && JS_SP1->type == JS_INTEGER)
    {
      JS_SP2->u.vinteger = ((JSInt32) JS_SP2->u.vinteger
			    >> (JSUInt32) JS_SP1->u.vinteger);
      JS_POP ();
    }
  else
    {
      JSInt32 l;
      JSUInt32 r;

      l = js_vm_to_int32 (vm, JS_SP2);
      r = (JSUInt32) js_vm_to_int32 (vm, JS_SP1);

      JS_SP2->u.vinteger = l >> r;
      JS_SP2->type = JS_INTEGER;
      JS_POP ();
    }
  NEXT ();

/* operand shift_rright (50) */
op_shift_rright:
  OPERAND (50);
  {
    JSInt32 l;
    JSUInt32 r;

    l = js_vm_to_int32 (vm, JS_SP2);
    r = (JSUInt32) js_vm_to_int32 (vm, JS_SP1);

    if (r > 0)
      JS_SP2->u.vinteger = (l & 0x7fffffff) >> r;
    else
      JS_SP2->u.vinteger = l;

    JS_SP2->type = JS_INTEGER;
    JS_POP ();
  }
  NEXT ();

/* operand iffalse (51) */
op_iffalse:
  OPERAND (51);
  READ_INT32 (i);
  if (JS_IS_FALSE (JS_SP1))
    SETPC_RELATIVE (i);
  JS_POP ();
  NEXT ();

/* operand iftrue (52) */
op_iftrue:
  OPERAND (52);
  READ_INT32 (i);
  if (JS_IS_TRUE (JS_SP1))
    SETPC_RELATIVE (i);
  JS_POP ();
  NEXT ();

/* operand call_method (53) */
op_call_method:
  OPERAND (53);
  /* Fetch the method symbol. */
  READ_INT32 (j);

  if (JS_SP1->type == JS_BUILTIN)
    {
      JS_SAVE_REGS ();
      if (JS_SP1->u.vbuiltin->info->method_proc)
	{
	  if ((*JS_SP1->u.vbuiltin->info->method_proc) (
				vm,
				JS_SP1->u.vbuiltin->info,
				JS_SP1->u.vbuiltin->instance_context, j,
				&builtin_result, JS_SP2)
	      == JS_PROPERTY_UNKNOWN)
	    ERROR ("call_method: unknown method");
	}
      else
	ERROR ("illegal builtin object for call_method");

      JS_COPY (JS_SP0, &builtin_result);
      JS_PUSH ();
      JS_MAYBE_GC ();
    }
  else if (JS_SP1->type == JS_OBJECT)
    {
      JSNode method;

      if (js_vm_object_load_property (vm, JS_SP1->u.vobject, j, &method)
	  == JS_PROPERTY_FOUND)
	{
	  /* The property has been defined in the object. */

	  if (method.type != JS_FUNC)
	    ERROR ("call_method: unknown method");

	  /* And once again.  We must do a subroutine call here. */
	  JS_SUBROUTINE_CALL (method.u.vfunction->implementation);
	}
      else
	/* Let our prototype handle this. */
	goto _op_call_method_try_proto;
    }
  else if (vm->prim[JS_SP1->type])
    {
      /* The primitive language types. */
    _op_call_method_try_proto:
      JS_SAVE_REGS ();
      if ((*vm->prim[JS_SP1->type]->method_proc) (vm, vm->prim[JS_SP1->type],
						  JS_SP1, j, &builtin_result,
						  JS_SP2)
	  == JS_PROPERTY_UNKNOWN)
	{
	  JSNode method;
	  int result = JS_PROPERTY_UNKNOWN;

	  /* Let's see if we can find it from the prototype. */
	  if (JS_SP1->type == JS_STRING && JS_SP1->u.vstring->prototype)
	    result = js_vm_object_load_property (vm,
						 JS_SP1->u.vstring->prototype,
						 j, &method);
	  else if (JS_SP1->type == JS_ARRAY && JS_SP1->u.varray->prototype)
	    result = js_vm_object_load_property (vm,
						 JS_SP1->u.varray->prototype,
						 j, &method);
	  else if (JS_SP1->type == JS_FUNC && JS_SP1->u.vfunction->prototype)
	    result
	      = js_vm_object_load_property (vm, JS_SP1->u.vfunction->prototype,
					    j, &method);

	  if (result == JS_PROPERTY_UNKNOWN || method.type != JS_FUNC)
	    ERROR ("call_method: unknown method");

	  /* Do the subroutine call. */
	  JS_SUBROUTINE_CALL (method.u.vfunction->implementation);
	}
      else
	{
	  JS_COPY (JS_SP0, &builtin_result);
	  JS_PUSH ();
	  JS_MAYBE_GC ();
	}
    }
  else
    ERROR ("illegal object for call_method");
  NEXT ();

/* operand jmp (54) */
op_jmp:
  OPERAND (54);
  READ_INT32 (i);
  SETPC_RELATIVE (i);
  NEXT ();

/* operand jsr (55) */
op_jsr:
  OPERAND (55);
  /* Call the global method. */
  {
    JSNode f;

    /* Fetch the function to our local variable. */
    JS_COPY (&f, JS_SP1);
    function = &f;

    /* Reset the `this' to null. */
    JS_SP1->type = JS_NULL;

    if (function->type == JS_BUILTIN
	&& function->u.vbuiltin->info->global_method_proc)
      {
	JS_SAVE_REGS ();
	(*function->u.vbuiltin->info->global_method_proc) (
				vm,
				function->u.vbuiltin->info,
				function->u.vbuiltin->instance_context,
				&builtin_result,
				JS_SP2);

	JS_COPY (JS_SP0, &builtin_result);
	JS_PUSH ();
      }
    else if (function->type == JS_FUNC)
      {
	JS_SUBROUTINE_CALL (function->u.vfunction->implementation);
      }
    else
      {
	sprintf (buf, "illegal function object in jsr");
	ERROR (buf);
      }
  }
  NEXT ();

/* operand return (56) */
op_return:
  OPERAND (56);
  if (fp->u.iptr == NULL)
    /* Return from the global scope. */
    DONE ();

  /* STACKFRAME */

  /* Check if the stack has been modified by min_args. */
  if (JS_ARGS_FIXP->u.args_fix.delta)
    {
      unsigned int delta = JS_ARGS_FIXP->u.args_fix.delta;

      /*
       * Yes it was.  Truncate it back to the state where it was
       * before the call.
       */

      memmove (JS_SP1 + delta, JS_SP1,
	       (fp - JS_SP0 + JS_ARGS_FIXP->u.args_fix.argc)
	       * sizeof (JSNode));

      sp += delta;
      fp += delta;
    }

  /* Set pc to the saved return address. */
#if 0
  if (fp[-3].type != JS_IPTR)
    ERROR ("can't find saved return address");
#endif
  pc = fp[-3].u.iptr;

  {
    void *old_fp;

    /* Save old frame pointer. */
#if 0
    if (fp->type != JS_IPTR)
      ERROR ("can't find saved frame pointer");
#endif
    old_fp = fp->u.iptr;

    /* Put return value to its correct location. */
    JS_COPY (fp, JS_SP1);

    /* Restore sp. */
    sp = &fp[-1];

    /* Restore frame pointer. */
    fp = old_fp;
  }
  NEXT ();

/* operand typeof (57) */
op_typeof:
  OPERAND (57);
  {
    char *typeof_name = "";	/* Initialized to make compiler quiet. */

    switch (JS_SP1->type)
      {
      case JS_UNDEFINED:
	typeof_name = "undefined";
	break;

      case JS_NULL:
	typeof_name = "object";
	break;

      case JS_BOOLEAN:
	typeof_name = "boolean";
	break;

      case JS_INTEGER:
      case JS_FLOAT:
      case JS_NAN:
	typeof_name = "number";
	break;

      case JS_STRING:
	typeof_name = "string";
	break;

      case JS_ARRAY:
	typeof_name = "#array";
	break;

      case JS_OBJECT:
	typeof_name = "object";
	break;

      case JS_SYMBOL:
	typeof_name = "#symbol";
	break;

      case JS_BUILTIN:
	typeof_name = "#builtin";
	break;

      case JS_FUNC:
	typeof_name = "function";
	break;

      case JS_IPTR:
	typeof_name = "#iptr";
	break;

      case JS_ARGS_FIX:
	typeof_name = "#argsfix";
	break;
      }

    js_vm_make_static_string (vm, JS_SP1, typeof_name, strlen (typeof_name));
    JS_MAYBE_GC ();
  }
  NEXT ();

/* operand new (58) */
op_new:
  OPERAND (58);
  /* Check object. */
  if (JS_SP1->type == JS_BUILTIN && JS_SP1->u.vbuiltin->info->new_proc)
    {
      JS_SAVE_REGS ();
      (*JS_SP1->u.vbuiltin->info->new_proc) (vm, JS_SP1->u.vbuiltin->info,
					     JS_SP2, JS_SP1);

      /* Push a dummy return value for the constructor.  This is ignored. */
      JS_SP0->type = JS_UNDEFINED;
      JS_PUSH ();
    }
  else if (JS_SP1->type == JS_FUNC)
    {
      JSObject *obj;
      JSNode f;
      JSNode prototype;

      /* The prototype is an object. */
      prototype.type = JS_OBJECT;

      /* Create the prototype for the function, if it is not defined. */
      if (JS_SP1->u.vfunction->prototype == NULL)
	{
	  JS_SP1->u.vfunction->prototype = js_vm_object_new (vm);

	  /* Set its __proto__ to point to Object's prototype.  */
	  prototype.u.vobject = vm->prim[JS_OBJECT]->prototype;
	  js_vm_object_store_property (vm, JS_SP1->u.vfunction->prototype,
				       vm->syms.s___proto__, &prototype);
	}

      /* Allocate a new object and set its prototype. */

      obj = js_vm_object_new (vm);

      prototype.u.vobject = JS_SP1->u.vfunction->prototype;
      js_vm_object_store_property (vm, obj, vm->syms.s___proto__, &prototype);

      /*
       * Basicly we do a jsr to the function given in JS_SP1.  But first,
       * we must set `this' pointer to the correct value.  See `jsr' for
       * the details.
       */
      JS_COPY (&f, JS_SP1);

      /* Replace func with the new object. */
      JS_SP1->type = JS_OBJECT;
      JS_SP1->u.vobject = obj;

      JS_SUBROUTINE_CALL (f.u.vfunction->implementation);
    }
  /* The primitive language types. */
  else if (vm->prim[JS_SP1->type])
    {
      JS_SAVE_REGS ();
      (*vm->prim[JS_SP1->type]->new_proc) (vm, vm->prim[JS_SP1->type], JS_SP2,
					   JS_SP1);
      JS_PUSH ();
    }
  else
    ERROR ("illegal object for new");

  JS_MAYBE_GC ();
  NEXT ();

/* operand delete_property (59) */
op_delete_property:
  OPERAND (59);
  /* Fetch the property symbol. */
  READ_INT32 (j);

  if (JS_SP1->type == JS_BUILTIN)
    {
      /*
       * XXX It should be possible to apply delete operand to builtin
       * XXX objects.
       */
      ERROR ("delete_property: JS_BUILTIN: not implemented yet");
    }
  else if (JS_SP1->type == JS_OBJECT)
    {
      js_vm_object_delete_property (vm, JS_SP1->u.vobject, j);
    }
  else if (JS_SP1->type == JS_NULL)
    {
      /* Delete a property from an object in the with-chain. */
      /* WITHCHAIN */
      ERROR ("delete_property: not implemented yet for the with-chain objects");
    }
  /* The primitive language types. */
  /*
   * XXX Since we can't delete properties from builtins, we can't delete
   * XXX them from the primitive language types.
   */
  else
    ERROR ("illegal object for delete_property");

  /* The delete operand returns an undefined value. */
  JS_SP1->type = JS_UNDEFINED;
  NEXT ();

/* operand delete_array (60) */
op_delete_array:
  OPERAND (60);
  if (JS_SP2->type == JS_BUILTIN)
    {
      ERROR ("delete_array: JS_BUILTIN: not implemented yet");
    }
  else if (JS_SP2->type == JS_OBJECT)
    {
      js_vm_object_delete_array (vm, JS_SP2->u.vobject, JS_SP1);
      JS_POP ();
    }
  else if (JS_SP2->type == JS_ARRAY)
    {
      if (JS_SP1->type == JS_INTEGER)
	{
	  if (0 <= JS_SP1->u.vinteger
	      && JS_SP1->u.vinteger < JS_SP2->u.varray->length)
	    JS_SP2->u.varray->data[JS_SP1->u.vinteger].type = JS_UNDEFINED;
	  JS_POP ();
	}
      else
	ERROR ("illegal array index in delete_array");
    }
  else
    ERROR ("illegal object for delete_array");

  /* The delete operand returns an undefined value. */
  JS_SP1->type = JS_UNDEFINED;
  NEXT ();

/* operand locals (61) */
op_locals:
  OPERAND (61);
  READ_INT16 (i);
  if (sp - i - JS_RESERVE_STACK_FOR_FUNCTION < vm->stack)
    ERROR ("stack overflow");

  for (; i > 0; i--)
    {
      JS_SP0->type = JS_UNDEFINED;
      JS_PUSH ();
    }
  NEXT ();

/* operand min_args (62) */
op_min_args:
  OPERAND (62);
  READ_INT8 (i);

  if (JS_SP1->u.vinteger < i)
    {
      unsigned int delta = i - JS_SP1->u.vinteger;
      unsigned int argc = JS_SP1->u.vinteger;

      memmove (JS_SP1 - delta, JS_SP1, (fp - JS_SP0 + argc) * sizeof (JSNode));
      sp -= delta;
      fp -= delta;

      /* Fill up the fix_args slot. */
      JS_ARGS_FIXP->u.args_fix.argc = argc;
      JS_ARGS_FIXP->u.args_fix.delta = delta;

      for (; argc < i; argc++)
	JS_ARG (argc)->type = JS_UNDEFINED;
    }

  JS_POP ();
  NEXT ();

/* operand load_nth_arg (63) */
op_load_nth_arg:
  OPERAND (63);
  {
    int index = JS_SP1->u.vinteger;
    JS_COPY (JS_SP1, JS_ARG (index));
  }
  NEXT ();

/* operand with_push (64) */
op_with_push:
  OPERAND (64);
  if (JS_SP1->type != JS_OBJECT && JS_SP1->type != JS_BUILTIN)
    ERROR ("illegal object for with_push");

  /* WITHCHAIN */

  if (JS_WITHPTR->u.iptr == NULL)
    {
      JSNode *np;
      JSUIntAlign *ip = js_vm_alloc (vm,
				     sizeof (JSUIntAlign)
				     + sizeof (JSNode));
      *ip = 1;
      np = (JSNode *) ((unsigned char *) ip + sizeof (JSUIntAlign));

      JS_COPY (np, JS_SP1);
      JS_WITHPTR->u.iptr = ip;
    }
  else
    {
      JSNode *np;
      JSUIntAlign *ip = JS_WITHPTR->u.iptr;
      JSUIntAlign ui = *ip;

      ip = js_vm_realloc (vm, ip,
			  sizeof (JSUIntAlign)
			  + ((ui + 1) * sizeof (JSNode)));
      (*ip)++;
      np = (JSNode *) ((unsigned char *) ip + sizeof (JSUIntAlign));

      JS_COPY (&np[ui], JS_SP1);
      JS_WITHPTR->u.iptr = ip;
    }

  JS_POP ();
  NEXT ();

/* operand with_pop (65) */
op_with_pop:
  OPERAND (65);
  READ_INT8 (i);

  /* WITHCHAIN */

  {
    JSUIntAlign *ip = JS_WITHPTR->u.iptr;

    if (ip == NULL || *ip < i)
      ERROR ("with stack underflow in with_pop");

    *ip -= i;
  }
  NEXT ();

/* operand try_push (66) */
op_try_push:
  OPERAND (66);
  READ_INT32 (i);

  {
    JSErrorHandlerFrame *frame = js_calloc (vm, 1, sizeof (*frame));

    frame->next = vm->error_handler;
    frame->sp = sp;
    frame->fp = fp;
    frame->pc = pc;
    frame->pc_delta = i;
    vm->error_handler = frame;

    if (setjmp (vm->error_handler->error_jmp))
      {
	/* Ok, we caught an error. */

	/* Restore our state. */
	sp = vm->error_handler->sp;
	fp = vm->error_handler->fp;
	pc = vm->error_handler->pc;
	i = vm->error_handler->pc_delta;

	/* Push the thrown value to the stack. */
	JS_COPY (JS_SP0, &vm->error_handler->thrown);
	JS_PUSH ();

	/* Remove this handler frame. */
	frame = vm->error_handler;
	vm->error_handler = vm->error_handler->next;
	js_free (frame);

	/* Do the jump to the catch block. */
	SETPC_RELATIVE (i);
      }
  }
  NEXT ();

/* operand try_pop (67) */
op_try_pop:
  OPERAND (67);
  READ_INT8 (i);

  for (; i > 0; i--)
    {
      JSErrorHandlerFrame *frame = vm->error_handler;

      vm->error_handler = vm->error_handler->next;
      js_free (frame);
    }
  NEXT ();

/* operand throw (68) */
op_throw:
  OPERAND (68);
  {
    JSErrorHandlerFrame *f = vm->error_handler;

    if (f->sp == NULL)
      {
	JSNode cvt;
	int len;

	/*
	 * We are jumping to the C-toplevel.  Convert our thrown value
	 * to string and store it to the vm->error.
	 */
	js_vm_to_string (vm, JS_SP1, &cvt);

	len = cvt.u.vstring->len;
	if (len + 1 > sizeof (vm->error))
	  len = sizeof (vm->error) - 1;

	memcpy (vm->error, cvt.u.vstring->data, len);
	vm->error[len] = '\0';
      }
    else
      JS_COPY (&f->thrown, JS_SP1);

    longjmp (f->error_jmp, 1);

    /* NOTREACHED (I hope). */

    sprintf (buf, "VM: no valid error handler initialized%s",
	     JS_HOST_LINE_BREAK);
    js_iostream_write (vm->s_stderr, buf, strlen (buf));
    js_iostream_flush (vm->s_stderr);

    abort ();
  }
  NEXT ();

/* operand iffalse_b (69) */
op_iffalse_b:
  OPERAND (69);
  READ_INT32 (i);
  if (!JS_SP1->u.vboolean)
    SETPC_RELATIVE (i);
  JS_POP ();
  NEXT ();

/* operand iftrue_b (70) */
op_iftrue_b:
  OPERAND (70);
  READ_INT32 (i);
  if (JS_SP1->u.vboolean)
    SETPC_RELATIVE (i);
  JS_POP ();
  NEXT ();

/* operand add_1_i (71) */
op_add_1_i:
  OPERAND (71);
  JS_SP1->u.vinteger++;
  NEXT ();

/* operand add_2_i (72) */
op_add_2_i:
  OPERAND (72);
  JS_SP1->u.vinteger += 2;
  NEXT ();

/* operand load_global_w (73) */
op_load_global_w:
  OPERAND (73);
  READ_INT32 (j);
  {
    int found = 0;

    /* Loop over the with chain. */
    /* WITHCHAIN */
    if (JS_WITHPTR->u.iptr)
      {
	JSUIntAlign *uip = JS_WITHPTR->u.iptr;
	JSUIntAlign ui = *uip;
	JSNode *wp = (JSNode *) ((unsigned char *) uip
				 + sizeof (JSUIntAlign));

	for (i = 0; i < ui; i++)
	  {
	    JSNode *w = &wp[i];
	    int result = JS_PROPERTY_UNKNOWN;

	    if (w->type == JS_BUILTIN)
	      {
		JS_SAVE_REGS ();
		if (w->u.vbuiltin->info->property_proc)
		  result = (*w->u.vbuiltin->info->property_proc) (
					vm,
					w->u.vbuiltin->info,
					w->u.vbuiltin->instance_context,
					j, 0, &builtin_result);
	      }
	    else if (w->type == JS_OBJECT)
	      {
		result = js_vm_object_load_property (vm, w->u.vobject, j,
						     &builtin_result);
	      }
	    else
	      ERROR ("corrupted with-chain in load_global");

	    if (result == JS_PROPERTY_FOUND)
	      {
		JS_COPY (JS_SP0, &builtin_result);
		JS_PUSH ();
		found = 1;
		break;
	      }
	  }
      }

    if (!found)
      {
	/* Use the global value. */
	JS_COPY (JS_SP0, JS_GLOBAL (j));
	JS_PUSH ();

	if (vm->warn_undef && JS_SP1->type == JS_UNDEFINED)
	  {
	    sprintf (buf, "VM: warning: using undefined global `%s'%s",
		     js_vm_symname (vm, j), JS_HOST_LINE_BREAK);
	    js_iostream_write (vm->s_stderr, buf, strlen (buf));
	  }
      }
  }
  NEXT ();

/* operand jsr_w (74) */
op_jsr_w:
  OPERAND (74);
  /* Read the subroutine symbol index. */
  READ_INT32 (j);

  {
    int found = 0;

    /* Loop over the with-chain. */
    /* WITHCHAIN */
    if (JS_WITHPTR->u.iptr)
      {
	JSUIntAlign *uip = JS_WITHPTR->u.iptr;
	JSUIntAlign ui = *uip;
	JSNode *wp = (JSNode *) ((unsigned char *) uip
				 + sizeof (JSUIntAlign));

	for (i = 0; i < ui; i++)
	  {
	    JSNode *w = &wp[i];
	    int result = JS_PROPERTY_UNKNOWN;

	    if (w->type == JS_BUILTIN)
	      {
		JS_SAVE_REGS ();
		if (w->u.vbuiltin->info->method_proc)
		  result = (*w->u.vbuiltin->info->method_proc) (
					vm,
					w->u.vbuiltin->info,
					w->u.vbuiltin->instance_context, j,
					&builtin_result, JS_SP2);
		JS_MAYBE_GC ();

		if (result == JS_PROPERTY_FOUND)
		  {
		    JS_COPY (JS_SP0, &builtin_result);
		    JS_PUSH ();
		  }
	      }
	    else if (w->type == JS_OBJECT)
	      {
		JSNode method;

		js_vm_object_load_property (vm, w->u.vobject, j, &method);
		if (method.type == JS_FUNC)
		  {
		    result = JS_PROPERTY_FOUND;

		    /* The object defines the method.  Do a subroutine call. */

		    /* First: replace the null `this' with `w'. */
		    JS_COPY (JS_SP1, w);

		    /* Then, do the normal subroutine call. */
		    JS_SUBROUTINE_CALL (method.u.vfunction->implementation);
		  }
	      }
	    else
	      ERROR ("corrupted with-chain in jsr_w");

	    if (result == JS_PROPERTY_FOUND)
	      {
		found = 1;
		break;
	      }
	  }
      }

    if (!found)
      {
	JSNode f;

	/* Call the global method. */
	JS_COPY (&f, JS_SP1);
	function = &f;

	/* Reset the `this' to null. */
	JS_SP1->type = JS_NULL;

	if (function->type == JS_BUILTIN
	    && function->u.vbuiltin->info->global_method_proc)
	  {
	    JS_SAVE_REGS ();
	    (*function->u.vbuiltin->info->global_method_proc) (
				vm,
				function->u.vbuiltin->info,
				function->u.vbuiltin->instance_context,
				&builtin_result,
				JS_SP2);

	    JS_COPY (JS_SP0, &builtin_result);
	    JS_PUSH ();
	  }
	else if (function->type == JS_FUNC)
	  {
	    JS_SUBROUTINE_CALL (function->u.vfunction->implementation);
	  }
	else
	  {
	    sprintf (buf, "symbol `%s' is undefined as function",
		     js_vm_symname (vm, j));
	    ERROR (buf);
	  }
      }
  }
  NEXT ();

