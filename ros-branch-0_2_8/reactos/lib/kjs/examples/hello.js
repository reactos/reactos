function main ()
{
  System.stdout.writeln ("main(): enter");

  System.stdout.writeln ("calling our `hello' global...");
  hello ();

  System.stdout.writeln ("calling our `Hello.show' method...");
  Hello.show ();
  try
    {
      Hello.fail ();
    }
  catch (msg)
    {
      System.stdout.writeln (msg);
    }

  System.stdout.writeln ("Hello.msg=" + Hello.msg);
  try
    {
      Hello.msg = "foo";
    }
  catch (msg)
    {
      System.stdout.writeln (msg);
    }

  var h1 = new Hello ("Hello, mtr!");
  h1.show ();

  Hello.investigate (h1);

  var h2 = h1.copy ();
  h2.show ();
  Hello.investigate (h2);

  System.stdout.writeln ("main(): leave");
}

main ();


/*
Local variables:
mode: c
End:
*/
