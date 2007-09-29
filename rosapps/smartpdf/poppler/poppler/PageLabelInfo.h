#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include <goo/gtypes.h>
#include <goo/GooList.h>
#include <goo/GooString.h>
#include <Object.h>

class PageLabelInfo {
public:
  PageLabelInfo(Object *tree, int numPages);
  ~PageLabelInfo();
  GBool labelToIndex(GooString *label, int *index);
  GBool indexToLabel(int index, GooString *label);

private:
  void parse(Object *tree);

private:
  struct Interval {
    Interval(Object *dict, int baseA);
    ~Interval();
    GooString *prefix;
    enum NumberStyle {
      None,
      Arabic,
      LowercaseRoman,
      UppercaseRoman,
      UppercaseLatin,
      LowercaseLatin
    } style;
    int first, base, length;
  };

  GooList intervals;
};
