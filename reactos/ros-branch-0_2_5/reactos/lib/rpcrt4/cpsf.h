/*
 * COM proxy definitions
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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

#ifndef __WINE_CPSF_H
#define __WINE_CPSF_H

HRESULT WINAPI StdProxy_Construct(REFIID riid,
				  LPUNKNOWN pUnkOuter,
				  PCInterfaceName name,
				  CInterfaceProxyVtbl *vtbl,
				  CInterfaceStubVtbl *svtbl,
				  LPPSFACTORYBUFFER pPSFactory,
				  LPRPCPROXYBUFFER *ppProxy,
				  LPVOID *ppvObj);
HRESULT WINAPI StdProxy_GetChannel(LPVOID iface,
				   LPRPCCHANNELBUFFER *ppChannel);
HRESULT WINAPI StdProxy_GetIID(LPVOID iface,
			       const IID **piid);

HRESULT WINAPI CStdStubBuffer_Construct(REFIID riid,
					LPUNKNOWN pUnkServer,
					PCInterfaceName name,
					CInterfaceStubVtbl *vtbl,
					LPPSFACTORYBUFFER pPSFactory,
					LPRPCSTUBBUFFER *ppStub);

#endif  /* __WINE_CPSF_H */
