 /*
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
 *
 * ReactOS emulation layer betwin wine and windows api for directx
 * This transform wine specfiy api to native reactos/windows api
 * wine have done some hack to geting dsound working. But does
 * hack does not work on windows or reactos. It need to warp thuse
 * api hack to true native api.
 *
 * this include file really need to be clean up.
 *
 * copyright 2004 by magnus olsen
 */

#ifdef __REACTOS__
#include <mmsystem.h>

// wine spec
#define MAXWAVEDRIVERS	10
#define MAXMIDIDRIVERS	10
#define MAXAUXDRIVERS	10
#define MAXMCIDRIVERS	32
#define MAXMIXERDRIVERS	10

/* where */
#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(x) (x)
#else
#define _HRESULT_TYPEDEF_(x) ((HRESULT)x)
#endif

/* wine own api */
#define DRV_QUERYDSOUNDIFACE		(DRV_RESERVED + 20)
#define DRV_QUERYDSOUNDDESC		    (DRV_RESERVED + 21)

#define WineWaveOutMessage          RosWineWaveOutMessage
#define WineWaveInMessage          RosWineWaveInMessage

#else
#define WineWaveOutMessage        WaveOutMessage
#define WineWaveInMessage          WaveInMessage
#endif


/* dxroslayers prototypes */
void dxGetGuidFromString( char *in_str, GUID *guid );

DWORD dxrosdrv_drv_querydsounddescss(int type, HWAVEOUT hwo_out,HWAVEIN  hwo_in, PDSDRIVERDESC pDESC);
DWORD dxrosdrv_drv_querydsoundiface(HWAVEIN wDevID, PIDSDRIVER* drv);

DWORD RosWineWaveOutMessage(HWAVEOUT  hwo, UINT, DWORD_PTR, DWORD_PTR);
DWORD RosWineWaveInMessage(HWAVEIN, UINT, DWORD_PTR, DWORD_PTR);
