/*
 * Unit test suite for *scanf functions.
 *
 * Copyright 2002 Uwe Bonnes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#include "wine/test.h"

static void test_sscanf( void )
{
    char buffer[100], buffer1[100];
    char format[20];
    int result, ret;
    char c;
    float res1= -82.6267f, res2= 27.76f, res11, res12;
    static const char pname[]=" St. Petersburg, Florida\n";
    int hour=21,min=59,sec=20;
    int  number,number_so_far;


    /* check EOF */
    strcpy(buffer,"");
    ret = sscanf(buffer, "%d", &result);
    ok( ret == EOF,"sscanf returns %x instead of %x\n", ret, EOF );

    /* check %x */
    strcpy(buffer,"0x519");
    ok( sscanf(buffer, "%x", &result) == 1, "sscanf failed\n"  );
    ok( result == 0x519,"sscanf reads %x instead of %x\n", result, 0x519 );

    strcpy(buffer,"0x51a");
    ok( sscanf(buffer, "%x", &result) == 1, "sscanf failed\n" );
    ok( result == 0x51a ,"sscanf reads %x instead of %x\n", result, 0x51a );

    strcpy(buffer,"0x51g");
    ok( sscanf(buffer, "%x", &result) == 1, "sscanf failed\n" );
    ok( result == 0x51, "sscanf reads %x instead of %x\n", result, 0x51 );

    /* check % followed by any char */
    strcpy(buffer,"\"%12@");
    strcpy(format,"%\"%%%d%@");  /* work around gcc format check */
    ok( sscanf(buffer, format, &result) == 1, "sscanf failed\n" );
    ok( result == 12, "sscanf reads %x instead of %x\n", result, 12 );

    /* Check float */
    ret = sprintf(buffer,"%f %f",res1, res2);
    ret = sscanf(buffer,"%f%f",&res11, &res12);
    ok( (res11 == res1) && (res12 == res2), "Error reading floats\n");

    /* check strings */
    ret = sprintf(buffer," %s", pname);
    ret = sscanf(buffer,"%*c%[^\n]",buffer1);
    ok( ret == 1, "Error with format \"%s\"\n","%*c%[^\n]");
    ok( strncmp(pname,buffer1,strlen(buffer1)) == 0, "Error with \"%s\" \"%s\"\n",pname, buffer1);

    ret = sscanf("abcefgdh","%*[a-cg-e]%c",&buffer[0]);
    ok( ret == 1, "Error with format \"%s\"\n","%*[a-cg-e]%c");
    ok( buffer[0] == 'd', "Error with \"abcefgdh\" \"%c\"\n", buffer[0]);

    ret = sscanf("abcefgdh","%*[a-cd-dg-e]%c",&buffer[0]);
    ok( ret == 1, "Error with format \"%s\"\n","%*[a-cd-dg-e]%c");
    ok( buffer[0] == 'h', "Error with \"abcefgdh\" \"%c\"\n", buffer[0]);

    /* check digits */
    ret = sprintf(buffer,"%d:%d:%d",hour,min,sec);
    ret = sscanf(buffer,"%d%n",&number,&number_so_far);
    ok(ret == 1 , "problem with format arg \"%%d%%n\"\n");
    ok(number == hour,"Read wrong arg %d instead of %d\n",number, hour);
    ok(number_so_far == 2,"Read wrong arg for \"%%n\" %d instead of 2\n",number_so_far);

    ret = sscanf(buffer+2,"%*c%n",&number_so_far);
    ok(ret == 0 , "problem with format arg \"%%*c%%n\"\n");
    ok(number_so_far == 1,"Read wrong arg for \"%%n\" %d instead of 2\n",number_so_far);

    /* Check %i according to bug 1878 */
    strcpy(buffer,"123");
    ret = sscanf(buffer, "%i", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 123, "Wrong number read\n");
    ret = sscanf(buffer, "%d", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 123, "Wrong number read\n");

    /* Check %c */
    strcpy(buffer,"a");
    c = 0x55;
    ret = sscanf(buffer, "%c", &c);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(c == 'a', "Field incorrect: '%c'\n", c);

    strcpy(buffer," a");
    c = 0x55;
    ret = sscanf(buffer, "%c", &c);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(c == ' ', "Field incorrect: '%c'\n", c);

    strcpy(buffer,"18:59");
    c = 0x55;
    ret = sscanf(buffer, "%d:%d%c", &hour, &min, &c);
    ok(ret == 2, "Wrong number of arguments read: %d\n", ret);
    ok(hour == 18, "Field 1 incorrect: %d\n", hour);
    ok(min == 59, "Field 2 incorrect: %d\n", min);
    ok(c == 0x55, "Field 3 incorrect: 0x%02x\n", c);

    /* Check %n (also whitespace in format strings and %s) */
    buffer[0]=0; buffer1[0]=0;
    ret = sscanf("abc   def", "%s %n%s", buffer, &number_so_far, buffer1);
    ok(strcmp(buffer, "abc")==0, "First %%s read incorrectly: %s\n", buffer);
    ok(strcmp(buffer1,"def")==0, "Second %%s read incorrectly: %s\n", buffer1);
    ok(number_so_far==6, "%%n yielded wrong result: %d\n", number_so_far);
    ok(ret == 2, "%%n shouldn't count as a conversion: %d\n", ret);
}

START_TEST(scanf)
{
    test_sscanf();
}
