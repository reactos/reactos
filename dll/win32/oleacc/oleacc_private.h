/*
 * Copyright 2014 Piotr Caban for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#pragma once

#include "oleacc_classes.h"

HRESULT create_client_object(HWND, const IID*, void**) DECLSPEC_HIDDEN;
HRESULT create_window_object(HWND, const IID*, void**) DECLSPEC_HIDDEN;
HRESULT get_accpropservices_factory(REFIID, void**) DECLSPEC_HIDDEN;

int convert_child_id(VARIANT *v) DECLSPEC_HIDDEN;
