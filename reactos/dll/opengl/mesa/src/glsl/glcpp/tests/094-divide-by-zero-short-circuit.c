/* glcpp is generating a division-by-zero error for this case.  It's
 * easy to argue that it should be short-circuiting the evaluation and
 * not generating the diagnostic (which happens to be what gcc does).
 * But it doesn't seem like we should force this behavior on our
 * pre-processor, (and, as always, the GLSL specification of the
 * pre-processor is too vague on this point).
 *
 * If a short-circuit evaluation optimization does get added to the
 * pre-processor then it would legitimate to update the expected file
 * for this test.
*/
#if 1 || (1 / 0)
#endif
