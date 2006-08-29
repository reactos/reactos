function main ()
{
  System.stdout.writeln ("main(): enter");

  System.stdout.writeln (("fubar").getType().toString());
  System.stdout.writeln ((new Object()).getType().toString());

  System.stdout.writeln ("main(): leave");
}

main ();


/*
Local variables:
mode: c
End:
*/
