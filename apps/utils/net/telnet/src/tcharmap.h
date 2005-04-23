// This is a simple class to handle character maps
// (Paul Brannan 6/25/98)

#ifndef __TCHARMAP_H
#define __TCHARMAP_H

class TCharmap {
private:
	char *map[256];
	char *current_map;
public:
	TCharmap();
	~TCharmap();

	void init() {}

	char translate(char c, char mapchar) {
		if(map[(unsigned char)mapchar]) return map[(unsigned char)mapchar][(unsigned char)c];
		return c;
	}
	char translate(char c) {
		return current_map[(unsigned char)c];
	}

	void setmap(char mapchar) {
		if(map[(unsigned char)mapchar]) current_map = map[(unsigned char)mapchar];
	}

	void translate_buffer(char *start, char *end) {
		while(start < end) {
			*start = translate(*start);
			start++;
		}
	}

	void modmap(char pos, char mapchar, char c);

	int enabled;
};

#endif
