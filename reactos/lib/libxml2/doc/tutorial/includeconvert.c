<![CDATA[
#include <string.h>
#include <libxml/parser.h>


unsigned char*
convert (unsigned char *in, char *encoding)
{
	unsigned char *out;
        int ret,size,out_size,temp;
        xmlCharEncodingHandlerPtr handler;

        size = (int)strlen(in)+1; 
        out_size = size*2-1; 
        out = malloc((size_t)out_size); 

        if (out) {
                handler = xmlFindCharEncodingHandler(encoding);
                
                if (!handler) {
                        free(out);
                        out = NULL;
                }
        }
        if (out) {
                temp=size-1;
                ret = handler->input(out, &out_size, in, &temp);
                if (ret || temp-size+1) {
                        if (ret) {
                                printf("conversion wasn't successful.\n");
                        } else {
                                printf("conversion wasn't successful. converted: %i octets.\n",temp);
                        }
                        free(out);
                        out = NULL;
                } else {
                        out = realloc(out,out_size+1); 
                        out[out_size]=0; /*null terminating out*/
                        
                }
        } else {
                printf("no mem\n");
        }
        return (out);
}	


int
main(int argc, char **argv) {

	unsigned char *content, *out;
	xmlDocPtr doc;
	xmlNodePtr rootnode;
	char *encoding = "ISO-8859-1";
	
		
	if (argc <= 1) {
		printf("Usage: %s content\n", argv[0]);
		return(0);
	}

	content = argv[1];

	out = convert(content, encoding);

	doc = xmlNewDoc ("1.0");
	rootnode = xmlNewDocNode(doc, NULL, (const xmlChar*)"root", out);
	xmlDocSetRootElement(doc, rootnode);

	xmlSaveFormatFileEnc("-", doc, encoding, 1);
	return (1);
}
]]>
