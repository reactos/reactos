size_t wcsnlen(const wchar_t * s, size_t count)
{
      
        unsigned int len=0;

        while(s[len]!=0 && len < count) {
                len++;
        };
        return len;
}
