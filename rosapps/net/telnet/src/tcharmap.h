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
		if(map[mapchar]) return map[mapchar][(unsigned char)c];
		return c;
	}
	char translate(char c) {
		return current_map[(unsigned char)c];
	}

	void setmap(char mapchar) {
		if(map[mapchar]) current_map = map[mapchar];
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
