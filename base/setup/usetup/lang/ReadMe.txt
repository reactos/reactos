Some source files here are converted by code7bit.

code7bit: https://github.com/katahiromz/code7bit

code7bit makes the string literals 8-bit clean.
If source contains any non-8-bit-clean strings,
non-Latin developer cannot build the project on MSVC.

8-bit clean
https://en.wikipedia.org/wiki/8-bit_clean

To edit a file converted by code7bit, you have to revert it at first.
To revert, please execute code7bit -r <file>.

After edit, to convert again, please execute code7bit -c <file>.
