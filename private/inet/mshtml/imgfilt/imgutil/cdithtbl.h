class CDitherTable
{
   friend class CDitherToRGB8;

public:
   CDitherTable();
   ~CDitherTable();

   BOOL Match( ULONG nColors, const RGBQUAD* prgbColors );
   HRESULT SetColors( ULONG nColors, const RGBQUAD* prgbColors );

protected:
   HRESULT BuildInverseMap();

   void inv_cmap( int colors, RGBQUAD *colormap, int bits, ULONG* dist_buf, 
      BYTE* rgbmap );
   int redloop();
   int greenloop( int restart );
   int blueloop( int restart );
   void maxfill( ULONG* buffer, long side );

public:
   BYTE m_abInverseMap[32768];

protected:
   ULONG m_nRefCount;
   ULONG m_nColors;
   RGBQUAD m_argbColors[256];
   ULONG* m_pnDistanceBuffer;

// Vars that were global in the original code
   int bcenter, gcenter, rcenter;
   long gdist, rdist, cdist;
   long cbinc, cginc, crinc;
   ULONG* gdp;
   ULONG* rdp;
   ULONG* cdp;
   BYTE* grgbp;
   BYTE* rrgbp;
   BYTE* crgbp;
   int gstride, rstride;
   long x, xsqr, colormax;
   int cindex;

// Static locals from the original redloop().  Good coding at its finest.
   long rxx;

// Static locals from the original greenloop()
   int greenloop_here;
   int greenloop_min;
   int greenloop_max;
   int greenloop_prevmin;
   int greenloop_prevmax;
   long ginc;
   long gxx;
   long gcdist;
   ULONG* gcdp;
   BYTE* gcrgbp;

// Static locals from the original blueloop()
   int blueloop_here;
   int blueloop_min;
   int blueloop_max;
   int blueloop_prevmin;
   int blueloop_prevmax;
   long binc;
};
