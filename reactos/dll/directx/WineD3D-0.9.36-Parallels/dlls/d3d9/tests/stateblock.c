/*
 * Copyright (C) 2005 Henri Verbeet
 * Copyright (C) 2006 Ivan Gyurdiev
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

#define COBJMACROS
#include <d3d9.h>
#include "wine/test.h"

static HMODULE d3d9_handle = 0;

static DWORD texture_stages;

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    return CreateWindow("d3d9_test_wc", "d3d9_test",
            0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static HRESULT init_d3d9(
    IDirect3DDevice9** device,
    D3DPRESENT_PARAMETERS* device_pparams)
{
    IDirect3D9 * (__stdcall * d3d9_create)(UINT SDKVersion) = 0;
    IDirect3D9 *d3d9_ptr = 0;
    HRESULT hres;
    HWND window;

    d3d9_create = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    ok(d3d9_create != NULL, "Failed to get address of Direct3DCreate9\n");
    if (!d3d9_create) return E_FAIL;
    
    d3d9_ptr = d3d9_create(D3D_SDK_VERSION);
    ok(d3d9_ptr != NULL, "Failed to create IDirect3D9 object\n");
    if (!d3d9_ptr) return E_FAIL;

    window = create_window();

    ZeroMemory(device_pparams, sizeof(D3DPRESENT_PARAMETERS));
    device_pparams->Windowed = TRUE;
    device_pparams->hDeviceWindow = window;
    device_pparams->SwapEffect = D3DSWAPEFFECT_DISCARD;

    hres = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, device_pparams, device);
    ok(hres == D3D_OK, "IDirect3D_CreateDevice returned: 0x%x\n", hres);
    return hres;
}

static void test_begin_end_state_block(IDirect3DDevice9 *device_ptr)
{
    HRESULT hret = 0;
    IDirect3DStateBlock9 *state_block_ptr = 0;

    /* Should succeed */
    hret = IDirect3DDevice9_BeginStateBlock(device_ptr);
    ok(hret == D3D_OK, "BeginStateBlock returned: hret 0x%x. Expected hret 0x%x. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) return;

    /* Calling BeginStateBlock while recording should return D3DERR_INVALIDCALL */
    hret = IDirect3DDevice9_BeginStateBlock(device_ptr);
    ok(hret == D3DERR_INVALIDCALL, "BeginStateBlock returned: hret 0x%x. Expected hret 0x%x. Aborting.\n", hret, D3DERR_INVALIDCALL);
    if (hret != D3DERR_INVALIDCALL) return;

    /* Should succeed */
    state_block_ptr = (IDirect3DStateBlock9 *)0xdeadbeef;
    hret = IDirect3DDevice9_EndStateBlock(device_ptr, &state_block_ptr);
    ok(hret == D3D_OK && state_block_ptr != 0 && state_block_ptr != (IDirect3DStateBlock9 *)0xdeadbeef, 
        "EndStateBlock returned: hret 0x%x, state_block_ptr %p. "
        "Expected hret 0x%x, state_block_ptr != %p, state_block_ptr != 0xdeadbeef.\n", hret, state_block_ptr, D3D_OK, NULL);

    /* Calling EndStateBlock while not recording should return D3DERR_INVALIDCALL. state_block_ptr should not be touched. */
    state_block_ptr = (IDirect3DStateBlock9 *)0xdeadbeef;
    hret = IDirect3DDevice9_EndStateBlock(device_ptr, &state_block_ptr);
    ok(hret == D3DERR_INVALIDCALL && state_block_ptr == (IDirect3DStateBlock9 *)0xdeadbeef, 
        "EndStateBlock returned: hret 0x%x, state_block_ptr %p. "
        "Expected hret 0x%x, state_block_ptr 0xdeadbeef.\n", hret, state_block_ptr, D3DERR_INVALIDCALL);
}

/* ============================ State Testing Framework ========================== */

typedef struct state_test {
    const char* test_name;

    /* The initial data is usually the same
     * as the default data, but a write can have side effects.
     * The initial data is tested first, before any writes take place
     * The default data can be tested after a write */
    const void* initial_data;

    /* The default data is the standard state to compare
     * against, and restore to */
    const void* default_data;

    /* The test data is the experiment data to try
     * in - what we want to write
     * out - what windows will actually write (not necessarily the same)  */
    const void* test_data_in;
    const void* test_data_out;

    /* The poison data is the data to preinitialize the return buffer to */
    const void* poison_data;
    
    /* Return buffer */
    void* return_data;

    /* Size of the data samples above */
    unsigned int data_size;

    /* Test resource management handlers */
    HRESULT (*setup_handler) (struct state_test* test);
    void (*teardown_handler) (struct state_test* test);

    /* Test data handlers */
    void (*set_handler) (IDirect3DDevice9* device, const struct state_test* test, const void* data_in);
    void (*get_handler) (IDirect3DDevice9* device, const struct state_test* test, void* data_out);
    void (*print_handler) (const struct state_test* test, const void* data);

    /* Test arguments */
    const void* test_arg;

    /* Test-specific context data */
    void* test_context;

} state_test;

/* See below for explanation of the flags */
#define EVENT_OK             0x00
#define EVENT_CHECK_DEFAULT  0x01 
#define EVENT_CHECK_INITIAL  0x02
#define EVENT_CHECK_TEST     0x04
#define EVENT_ERROR          0x08
#define EVENT_APPLY_DATA     0x10

typedef struct event {
   int (*event_fn) (IDirect3DDevice9* device, void* arg);
   int status;
} event;

/* This is an event-machine, which tests things.
 * It tests get and set operations for a batch of states, based on
 * results from the event function, which directs what's to be done */

static void execute_test_chain(
    IDirect3DDevice9* device,
    state_test* test,
    unsigned int ntests,
    event *event,
    unsigned int nevents,
    void* event_arg) {

    int outcome;
    unsigned int i = 0, j;

    /* For each queued event */
    for (j=0; j < nevents; j++) {

        /* Execute the next event handler (if available) or just set the supplied status */
        outcome = event[j].status;
        if (event[j].event_fn)
            outcome |= event[j].event_fn(device, event_arg);

        /* Now verify correct outcome depending on what was signaled by the handler.
         * An EVENT_CHECK_TEST signal means check the returned data against the test_data (out).
         * An EVENT_CHECK_DEFAULT signal means check the returned data against the default_data.
         * An EVENT_CHECK_INITIAL signal means check the returned data against the initial_data.
         * An EVENT_ERROR signal means the test isn't going to work, exit the event loop.
         * An EVENT_APPLY_DATA signal means load the test data (after checks) */

        if (outcome & EVENT_ERROR) {
            trace("Test %s, Stage %u in error state, aborting\n", test[i].test_name, j);
            break;

        } else if (outcome & EVENT_CHECK_TEST || outcome & EVENT_CHECK_DEFAULT || outcome & EVENT_CHECK_INITIAL) {

            for (i=0; i < ntests; i++) {

                memcpy(test[i].return_data, test[i].poison_data, test[i].data_size);
                test[i].get_handler(device, &test[i], test[i].return_data);

                if (outcome & EVENT_CHECK_TEST) {

                    BOOL test_failed = memcmp(test[i].test_data_out, test[i].return_data, test[i].data_size);
                    ok(!test_failed, "Test %s, Stage %u: returned data does not match test data [csize=%u]\n",
                        test[i].test_name, j, test[i].data_size);

                    if (test_failed && test[i].print_handler) {
                        trace("Returned data was:\n");
                        test[i].print_handler(&test[i], test[i].return_data);
                        trace("Test data was:\n");
                        test[i].print_handler(&test[i], test[i].test_data_out);
                    }
                }

                else if (outcome & EVENT_CHECK_DEFAULT) {
                  
                    BOOL test_failed = memcmp(test[i].default_data, test[i].return_data, test[i].data_size);
                    ok (!test_failed, "Test %s, Stage %u: returned data does not match default data [csize=%u]\n",
                        test[i].test_name, j, test[i].data_size);

                    if (test_failed && test[i].print_handler) {
                        trace("Returned data was:\n");
                        test[i].print_handler(&test[i], test[i].return_data);
                        trace("Default data was:\n");
                        test[i].print_handler(&test[i], test[i].default_data);
                    }
                }

                else if (outcome & EVENT_CHECK_INITIAL) {
                    
                    BOOL test_failed = memcmp(test[i].initial_data, test[i].return_data, test[i].data_size);
                    ok (!test_failed, "Test %s, Stage %u: returned data does not match initial data [csize=%u]\n",
                        test[i].test_name, j, test[i].data_size);

                    if (test_failed && test[i].print_handler) {
                        trace("Returned data was:\n");
                        test[i].print_handler(&test[i], test[i].return_data);
                        trace("Initial data was:\n");
                        test[i].print_handler(&test[i], test[i].initial_data);
                    }
                }
            }
        }

        if (outcome & EVENT_APPLY_DATA) {
            for (i=0; i < ntests; i++)
                test[i].set_handler(device, &test[i], test[i].test_data_in);
        }
     }

     /* Attempt to reset any changes made */
     for (i=0; i < ntests; i++)
         test[i].set_handler(device, &test[i], test[i].default_data);
}

typedef struct event_data {
    IDirect3DStateBlock9* stateblock;
    IDirect3DSurface9* original_render_target;
} event_data;

static int switch_render_target(
    IDirect3DDevice9* device,
    void* data) {
  
    HRESULT hret;
    D3DPRESENT_PARAMETERS present_parameters;
    event_data* edata = (event_data*) data;
    IDirect3DSwapChain9* swapchain = NULL;
    IDirect3DSurface9* backbuffer = NULL;

    /* Parameters for new swapchain */
    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    /* Create new swapchain */
    hret = IDirect3DDevice9_CreateAdditionalSwapChain(device, &present_parameters, &swapchain);
    ok (hret == D3D_OK, "CreateAdditionalSwapChain returned %#x.\n", hret);
    if (hret != D3D_OK) goto error;

    /* Get its backbuffer */
    hret = IDirect3DSwapChain9_GetBackBuffer(swapchain, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok (hret == D3D_OK, "GetBackBuffer returned %#x.\n", hret);
    if (hret != D3D_OK) goto error;

    /* Save the current render target */
    hret = IDirect3DDevice9_GetRenderTarget(device, 0, &edata->original_render_target); 
    ok (hret == D3D_OK, "GetRenderTarget returned %#x.\n", hret);
    if (hret != D3D_OK) goto error;

    /* Set the new swapchain's backbuffer as a render target */
    hret = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok (hret == D3D_OK, "SetRenderTarget returned %#x.\n", hret);
    if (hret != D3D_OK) goto error;

    IUnknown_Release(backbuffer);
    IUnknown_Release(swapchain);
    return EVENT_OK;

    error:
    if (backbuffer) IUnknown_Release(backbuffer);
    if (swapchain) IUnknown_Release(swapchain);
    return EVENT_ERROR;
}

static int revert_render_target( 
    IDirect3DDevice9* device,
    void* data) {

    HRESULT hret;
    event_data* edata = (event_data*) data;
   
    /* Reset the old render target */
    hret = IDirect3DDevice9_SetRenderTarget(device, 0, edata->original_render_target);
    ok (hret == D3D_OK, "SetRenderTarget returned %#x.\n", hret);
    if (hret != D3D_OK) {
        IUnknown_Release(edata->original_render_target);
        return EVENT_ERROR;
    }

    IUnknown_Release(edata->original_render_target);
    return EVENT_OK;
}

static int begin_stateblock(
    IDirect3DDevice9* device,
    void* data) {

    HRESULT hret;

    data = NULL;
    hret = IDirect3DDevice9_BeginStateBlock(device);
    ok(hret == D3D_OK, "BeginStateBlock returned %#x.\n", hret);
    if (hret != D3D_OK) return EVENT_ERROR;
    return EVENT_OK;
}

static int end_stateblock(
    IDirect3DDevice9* device,
    void* data) {

    HRESULT hret;
    event_data* edata = (event_data*) data;

    hret = IDirect3DDevice9_EndStateBlock(device, &edata->stateblock);
    ok(hret == D3D_OK, "EndStateBlock returned %#x.\n", hret);
    if (hret != D3D_OK) return EVENT_ERROR;
    return EVENT_OK;
}

static int abort_stateblock(
    IDirect3DDevice9* device,
    void* data) {

    event_data* edata = (event_data*) data;

    IUnknown_Release(edata->stateblock);
    return EVENT_OK;
}

static int apply_stateblock(
    IDirect3DDevice9* device,
    void* data) {

    event_data* edata = (event_data*) data;
    HRESULT hret;

    hret = IDirect3DStateBlock9_Apply(edata->stateblock);
    ok(hret == D3D_OK, "Apply returned %#x.\n", hret);
    if (hret != D3D_OK) {
        IUnknown_Release(edata->stateblock);
        return EVENT_ERROR;
    }

    IUnknown_Release(edata->stateblock);
    return EVENT_OK;
}

static int capture_stateblock(
    IDirect3DDevice9* device,
    void* data) {

    HRESULT hret;
    event_data* edata = (event_data*) data;

    hret = IDirect3DStateBlock9_Capture(edata->stateblock);
    ok(hret == D3D_OK, "Capture returned %#x.\n", hret);
    if (hret != D3D_OK)
        return EVENT_ERROR;

    return EVENT_OK;
}

static void execute_test_chain_all(
    IDirect3DDevice9* device,
    state_test* test,
    unsigned int ntests) {

    unsigned int i;
    event_data arg;

    event read_events[] = {
        { NULL, EVENT_CHECK_INITIAL }
    };

    event write_read_events[] = {
        { NULL, EVENT_APPLY_DATA },
        { NULL, EVENT_CHECK_TEST }
    };

    event abort_stateblock_events[] = {
        { begin_stateblock, EVENT_APPLY_DATA },
        { end_stateblock, EVENT_OK },
        { abort_stateblock, EVENT_CHECK_DEFAULT }
    };

    event apply_stateblock_events[] = {
        { begin_stateblock, EVENT_APPLY_DATA },
        { end_stateblock, EVENT_OK },
        { apply_stateblock, EVENT_CHECK_TEST }
    };

    event capture_reapply_stateblock_events[] = {
          { begin_stateblock, EVENT_APPLY_DATA },
          { end_stateblock, EVENT_OK },
          { capture_stateblock, EVENT_CHECK_DEFAULT | EVENT_APPLY_DATA },
          { apply_stateblock, EVENT_CHECK_DEFAULT }
    };

    event rendertarget_switch_events[] = {
          { NULL, EVENT_APPLY_DATA },
          { switch_render_target, EVENT_CHECK_TEST },
          { revert_render_target, EVENT_OK } 
    };

    event rendertarget_stateblock_events[] = {
          { begin_stateblock, EVENT_APPLY_DATA },
          { switch_render_target, EVENT_CHECK_DEFAULT },
          { end_stateblock, EVENT_OK },
          { revert_render_target, EVENT_OK },
          { apply_stateblock, EVENT_CHECK_TEST }
    };

    /* Setup each test for execution */
    for (i=0; i < ntests; i++) {
        if (test[i].setup_handler(&test[i]) != D3D_OK) {
            ok(FALSE, "Test \"%s\" failed setup, aborting\n", test[i].test_name);
            return;
        }
    }

    trace("Running initial read state tests\n");
    execute_test_chain(device, test, ntests, read_events, 1, NULL);

    trace("Running write-read state tests\n");
    execute_test_chain(device, test, ntests, write_read_events, 2, NULL);

    trace("Running stateblock abort state tests\n");
    execute_test_chain(device, test, ntests, abort_stateblock_events, 3, &arg);

    trace("Running stateblock apply state tests\n");
    execute_test_chain(device, test, ntests, apply_stateblock_events, 3, &arg);

    trace("Running stateblock capture/reapply state tests\n");
    execute_test_chain(device, test, ntests, capture_reapply_stateblock_events, 4, &arg);
  
    trace("Running rendertarget switch state tests\n");
    execute_test_chain(device, test, ntests, rendertarget_switch_events, 3, &arg);

    trace("Running stateblock apply over rendertarget switch interrupt tests\n");
    execute_test_chain(device, test, ntests, rendertarget_stateblock_events, 5, &arg);

    /* Cleanup resources */
    for (i=0; i < ntests; i++)
        test[i].teardown_handler(&test[i]);
}

/* =================== State test: Pixel and Vertex Shader constants ============ */

typedef struct shader_constant_data {
    int int_constant[4];     /* 1x4 integer constant */
    float float_constant[4]; /* 1x4 float constant */
    BOOL bool_constant[4];   /* 4x1 boolean constants */
} shader_constant_data;

typedef struct shader_constant_arg {
    unsigned int idx;
    BOOL pshader;
} shader_constant_arg;

typedef struct shader_constant_context {
    shader_constant_data return_data_buffer;
} shader_constant_context;

static void shader_constant_print_handler(
    const state_test* test,
    const void* data) {

    const shader_constant_data* scdata = data;

    trace("Integer constant = { %#x, %#x, %#x, %#x }\n",
        scdata->int_constant[0], scdata->int_constant[1],
        scdata->int_constant[2], scdata->int_constant[3]);

    trace("Float constant = { %f, %f, %f, %f }\n",
        scdata->float_constant[0], scdata->float_constant[1],
        scdata->float_constant[2], scdata->float_constant[3]);

    trace("Boolean constants = [ %#x, %#x, %#x, %#x ]\n",
        scdata->bool_constant[0], scdata->bool_constant[1],
        scdata->bool_constant[2], scdata->bool_constant[3]);
}

static void shader_constant_set_handler(
    IDirect3DDevice9* device, const state_test* test, const void* data) {

    HRESULT hret;
    const shader_constant_data* scdata = data;
    const shader_constant_arg* scarg = test->test_arg;
    unsigned int index = scarg->idx;

    if (!scarg->pshader) {
        hret = IDirect3DDevice9_SetVertexShaderConstantI(device, index, scdata->int_constant, 1);
        ok(hret == D3D_OK, "SetVertexShaderConstantI returned %#x.\n", hret);
        hret = IDirect3DDevice9_SetVertexShaderConstantF(device, index, scdata->float_constant, 1);
        ok(hret == D3D_OK, "SetVertexShaderConstantF returned %#x.\n", hret);
        hret = IDirect3DDevice9_SetVertexShaderConstantB(device, index, scdata->bool_constant, 4);
        ok(hret == D3D_OK, "SetVertexShaderConstantB returned %#x.\n", hret);

    } else {
        hret = IDirect3DDevice9_SetPixelShaderConstantI(device, index, scdata->int_constant, 1);
        ok(hret == D3D_OK, "SetPixelShaderConstantI returned %#x.\n", hret);
        hret = IDirect3DDevice9_SetPixelShaderConstantF(device, index, scdata->float_constant, 1);
        ok(hret == D3D_OK, "SetPixelShaderConstantF returned %#x.\n", hret);
        hret = IDirect3DDevice9_SetPixelShaderConstantB(device, index, scdata->bool_constant, 4);
        ok(hret == D3D_OK, "SetPixelShaderConstantB returned %#x.\n", hret);
    }
}

static void shader_constant_get_handler(
    IDirect3DDevice9* device, const state_test* test, void* data) {

    HRESULT hret;
    shader_constant_data* scdata = (shader_constant_data*) data;
    const shader_constant_arg* scarg = test->test_arg;
    unsigned int index = scarg->idx;

    if (!scarg->pshader) {
        hret = IDirect3DDevice9_GetVertexShaderConstantI(device, index, scdata->int_constant, 1);
        ok(hret == D3D_OK, "GetVertexShaderConstantI returned %#x.\n", hret);
        hret = IDirect3DDevice9_GetVertexShaderConstantF(device, index, scdata->float_constant, 1);
        ok(hret == D3D_OK, "GetVertexShaderConstantF returned %#x.\n", hret);
        hret = IDirect3DDevice9_GetVertexShaderConstantB(device, index, scdata->bool_constant, 4);
        ok(hret == D3D_OK, "GetVertexShaderConstantB returned %#x.\n", hret);

    } else {
        hret = IDirect3DDevice9_GetPixelShaderConstantI(device, index, scdata->int_constant, 1);
        ok(hret == D3D_OK, "GetPixelShaderConstantI returned %#x.\n", hret);
        hret = IDirect3DDevice9_GetPixelShaderConstantF(device, index, scdata->float_constant, 1);
        ok(hret == D3D_OK, "GetPixelShaderConstantF returned %#x.\n", hret);
        hret = IDirect3DDevice9_GetPixelShaderConstantB(device, index, scdata->bool_constant, 4);
        ok(hret == D3D_OK, "GetPixelShaderConstantB returned %#x.\n", hret);
    }
}

static const shader_constant_data shader_constant_poison_data = {
    { 0x1337c0de, 0x1337c0de, 0x1337c0de, 0x1337c0de },
    { 1.0f, 2.0f, 3.0f, 4.0f },
    { FALSE, TRUE, FALSE, TRUE }
};

static const shader_constant_data shader_constant_default_data = {
        { 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0, 0, 0, 0 }
};

static const shader_constant_data shader_constant_test_data = {
    { 0xdead0000, 0xdead0001, 0xdead0002, 0xdead0003 },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { TRUE, FALSE, FALSE, TRUE }
};

static HRESULT shader_constant_setup_handler(
    state_test* test) {

    shader_constant_context *ctx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(shader_constant_context));
    if (ctx == NULL) return E_FAIL;
    test->test_context = ctx;
     
    test->return_data = &ctx->return_data_buffer;
    test->test_data_in = &shader_constant_test_data;
    test->test_data_out = &shader_constant_test_data;
    test->default_data = &shader_constant_default_data;
    test->initial_data = &shader_constant_default_data;
    test->poison_data = &shader_constant_poison_data;

    test->data_size = sizeof(shader_constant_data);
    
    return D3D_OK;
}

static void shader_constant_teardown_handler(
    state_test* test) {
    
    HeapFree(GetProcessHeap(), 0, test->test_context);
}

static void shader_constants_queue_test(
    state_test* test,
    const shader_constant_arg* test_arg) {

    test->setup_handler = shader_constant_setup_handler;
    test->teardown_handler = shader_constant_teardown_handler;
    test->set_handler = shader_constant_set_handler;
    test->get_handler = shader_constant_get_handler;
    test->print_handler = shader_constant_print_handler;
    test->test_name = test_arg->pshader? "set_get_pshader_constants": "set_get_vshader_constants";
    test->test_arg = test_arg;
}

/* =================== State test: Lights ===================================== */

typedef struct light_data {
    D3DLIGHT9 light;
    BOOL enabled;
    HRESULT get_light_result;
    HRESULT get_enabled_result;
} light_data;

typedef struct light_arg {
    unsigned int idx;
} light_arg;

typedef struct light_context {
    light_data return_data_buffer;
} light_context;

static void light_print_handler(
    const state_test* test,
    const void* data) {

    const light_data* ldata = data;

    trace("Get Light return value: %#x\n", ldata->get_light_result);
    trace("Get Light enable return value: %#x\n", ldata->get_enabled_result);

    trace("Light Enabled = %u\n", ldata->enabled);
    trace("Light Type = %u\n", ldata->light.Type);
    trace("Light Diffuse = { %f, %f, %f, %f }\n",
        ldata->light.Diffuse.r, ldata->light.Diffuse.g,
        ldata->light.Diffuse.b, ldata->light.Diffuse.a);
    trace("Light Specular = { %f, %f, %f, %f}\n",
        ldata->light.Specular.r, ldata->light.Specular.g,
        ldata->light.Specular.b, ldata->light.Specular.a);
    trace("Light Ambient = { %f, %f, %f, %f }\n",
        ldata->light.Ambient.r, ldata->light.Ambient.g,
        ldata->light.Ambient.b, ldata->light.Ambient.a);
    trace("Light Position = { %f, %f, %f }\n",
        ldata->light.Position.x, ldata->light.Position.y, ldata->light.Position.z);
    trace("Light Direction = { %f, %f, %f }\n",
        ldata->light.Direction.x, ldata->light.Direction.y, ldata->light.Direction.z);
    trace("Light Range = %f\n", ldata->light.Range);
    trace("Light Fallof = %f\n", ldata->light.Falloff);
    trace("Light Attenuation0 = %f\n", ldata->light.Attenuation0);
    trace("Light Attenuation1 = %f\n", ldata->light.Attenuation1);
    trace("Light Attenuation2 = %f\n", ldata->light.Attenuation2);
    trace("Light Theta = %f\n", ldata->light.Theta);
    trace("Light Phi = %f\n", ldata->light.Phi);
}

static void light_set_handler(
    IDirect3DDevice9* device, const state_test* test, const void* data) {

    HRESULT hret;
    const light_data* ldata = data;
    const light_arg* larg = test->test_arg;
    unsigned int index = larg->idx;

    hret = IDirect3DDevice9_SetLight(device, index, &ldata->light);
    ok(hret == D3D_OK, "SetLight returned %#x.\n", hret);

    hret = IDirect3DDevice9_LightEnable(device, index, ldata->enabled);
    ok(hret == D3D_OK, "SetLightEnable returned %#x.\n", hret);
}

static void light_get_handler(
    IDirect3DDevice9* device, const state_test* test, void* data) {

    HRESULT hret;
    light_data* ldata = data;
    const light_arg* larg = test->test_arg;
    unsigned int index = larg->idx;

    hret = IDirect3DDevice9_GetLightEnable(device, index, &ldata->enabled);
    ldata->get_enabled_result = hret;

    hret = IDirect3DDevice9_GetLight(device, index, &ldata->light);
    ldata->get_light_result = hret;
}

static const light_data light_poison_data = 
    { { 0x1337c0de,
        { 7.0, 4.0, 2.0, 1.0 }, { 7.0, 4.0, 2.0, 1.0 }, { 7.0, 4.0, 2.0, 1.0 },
        { 3.3, 4.4, 5.5 }, { 6.6, 7.7, 8.8 },
        12.12, 13.13, 14.14, 15.15, 16.16, 17.17, 18.18 }, 1, 0x1337c0de, 0x1337c0de };

static const light_data light_default_data =
    { { D3DLIGHT_DIRECTIONAL,
        { 1.0, 1.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 },
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }, 0, D3D_OK, D3D_OK };

/* This is used for the initial read state (before a write causes side effects)
 * The proper return status is D3DERR_INVALIDCALL */
static const light_data light_initial_data =
    { { 0x1337c0de,
        { 7.0, 4.0, 2.0, 1.0 }, { 7.0, 4.0, 2.0, 1.0 }, { 7.0, 4.0, 2.0, 1.0 },
        { 3.3, 4.4, 5.5 }, { 6.6, 7.7, 8.8 },
        12.12, 13.13, 14.14, 15.15, 16.16, 17.17, 18.18 }, 1, D3DERR_INVALIDCALL, D3DERR_INVALIDCALL };

static const light_data light_test_data_in =
    { { 1,
        { 2.0, 2.0, 2.0, 2.0 }, { 3.0, 3.0, 3.0, 3.0 }, { 4.0, 4.0, 4.0, 4.0 },
        { 5.0, 5.0, 5.0 }, { 6.0, 6.0, 6.0 },
        7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 }, 1, D3D_OK, D3D_OK};

/* SetLight will use 128 as the "enabled" value */
static const light_data light_test_data_out = 
    { { 1,
        { 2.0, 2.0, 2.0, 2.0 }, { 3.0, 3.0, 3.0, 3.0 }, { 4.0, 4.0, 4.0, 4.0 },
        { 5.0, 5.0, 5.0 }, { 6.0, 6.0, 6.0 },
        7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 }, 128, D3D_OK, D3D_OK};

static HRESULT light_setup_handler(
    state_test* test) {
     
    light_context *ctx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(light_context));
    if (ctx == NULL) return E_FAIL;
    test->test_context = ctx;
 
    test->return_data = &ctx->return_data_buffer;
    test->test_data_in = &light_test_data_in;
    test->test_data_out = &light_test_data_out;
    test->default_data = &light_default_data;
    test->initial_data = &light_initial_data;
    test->poison_data = &light_poison_data;

    test->data_size = sizeof(light_data);
    
    return D3D_OK;
}

static void light_teardown_handler(
    state_test* test) {
    
    HeapFree(GetProcessHeap(), 0, test->test_context);
}

static void lights_queue_test(
    state_test* test,
    const light_arg* test_arg) {

    test->setup_handler = light_setup_handler;
    test->teardown_handler = light_teardown_handler;
    test->set_handler = light_set_handler;
    test->get_handler = light_get_handler;
    test->print_handler = light_print_handler;
    test->test_name = "set_get_light";
    test->test_arg = test_arg;
}

/* =================== State test: Transforms ===================================== */

typedef struct transform_data {

    D3DMATRIX view;
    D3DMATRIX projection;
    D3DMATRIX texture0;
    D3DMATRIX texture7;
    D3DMATRIX world0;
    D3DMATRIX world255;

} transform_data;

typedef struct transform_context {
    transform_data return_data_buffer;
} transform_context;

static inline void print_matrix(
    const char* name, const D3DMATRIX* matrix) {

    trace("%s Matrix = {\n", name);
    trace("    %f %f %f %f\n", U(*matrix).m[0][0], U(*matrix).m[1][0], U(*matrix).m[2][0], U(*matrix).m[3][0]);
    trace("    %f %f %f %f\n", U(*matrix).m[0][1], U(*matrix).m[1][1], U(*matrix).m[2][1], U(*matrix).m[3][1]);
    trace("    %f %f %f %f\n", U(*matrix).m[0][2], U(*matrix).m[1][2], U(*matrix).m[2][2], U(*matrix).m[3][2]);
    trace("    %f %f %f %f\n", U(*matrix).m[0][3], U(*matrix).m[1][3], U(*matrix).m[2][3], U(*matrix).m[3][3]);
    trace("}\n");
}

static void transform_print_handler(
    const state_test* test,
    const void* data) {

    const transform_data* tdata = data;

    print_matrix("View", &tdata->view);
    print_matrix("Projection", &tdata->projection);
    print_matrix("Texture0", &tdata->texture0);
    print_matrix("Texture7", &tdata->texture7);
    print_matrix("World0", &tdata->world0);
    print_matrix("World255", &tdata->world255);
}

static void transform_set_handler(
    IDirect3DDevice9* device, const state_test* test, const void* data) {

    HRESULT hret;
    const transform_data* tdata = data;

    hret = IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, &tdata->view);
    ok(hret == D3D_OK, "SetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, &tdata->projection);
    ok(hret == D3D_OK, "SetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, &tdata->texture0);
    ok(hret == D3D_OK, "SetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0 + texture_stages - 1, &tdata->texture7);
    ok(hret == D3D_OK, "SetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_SetTransform(device, D3DTS_WORLD, &tdata->world0);
    ok(hret == D3D_OK, "SetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_SetTransform(device, D3DTS_WORLDMATRIX(255), &tdata->world255);
    ok(hret == D3D_OK, "SetTransform returned %#x.\n", hret);
}

static void transform_get_handler(
    IDirect3DDevice9* device, const state_test* test, void* data) {

    HRESULT hret;
    transform_data* tdata = (transform_data*) data;

    hret = IDirect3DDevice9_GetTransform(device, D3DTS_VIEW, &tdata->view);
    ok(hret == D3D_OK, "GetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_GetTransform(device, D3DTS_PROJECTION, &tdata->projection);
    ok(hret == D3D_OK, "GetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_GetTransform(device, D3DTS_TEXTURE0, &tdata->texture0);
    ok(hret == D3D_OK, "GetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_GetTransform(device, D3DTS_TEXTURE0 + texture_stages - 1, &tdata->texture7);
    ok(hret == D3D_OK, "GetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &tdata->world0);
    ok(hret == D3D_OK, "GetTransform returned %#x.\n", hret);

    hret = IDirect3DDevice9_GetTransform(device, D3DTS_WORLDMATRIX(255), &tdata->world255);
    ok(hret == D3D_OK, "GetTransform returned %#x.\n", hret);
}

static const transform_data transform_default_data = {
      { { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } },
      { { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } },
      { { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } },
      { { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } },
      { { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } },
      { { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } } }
};

static const transform_data transform_poison_data = {
      { { { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
        9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0 } } },

      { { { 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
        25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0 } } },

      { { { 33.0, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0, 40.0,
        41.0, 42.0, 43.0, 44.0, 45.0, 46.0, 47.0, 48.0 } } },

      { { { 49.0, 50.0, 51.0, 52.0, 53.0, 54.0, 55.0, 56.0,
        57.0, 58.0, 59.0, 60.0, 61.0, 62.0, 63.0, 64.0 } } },

      { { { 64.0, 66.0, 67.0, 68.0, 69.0, 70.0, 71.0, 72.0,
        73.0, 74.0, 75.0, 76.0, 77.0, 78.0, 79.0, 80.0 } } },

      { { { 81.0, 82.0, 83.0, 84.0, 85.0, 86.0, 87.0, 88.0,
        89.0, 90.0, 91.0, 92.0, 93.0, 94.0, 95.0, 96.0} } },
};

static const transform_data transform_test_data = {
      { { { 1.2, 3.4, -5.6, 7.2, 10.11, -12.13, 14.15, -1.5,
        23.56, 12.89, 44.56, -1.0, 2.3, 0.0, 4.4, 5.5 } } },

      { { { 9.2, 38.7, -6.6, 7.2, 10.11, -12.13, 77.15, -1.5,
        23.56, 12.89, 14.56, -1.0, 12.3, 0.0, 4.4, 5.5 } } },

      { { { 10.2, 3.4, 0.6, 7.2, 10.11, -12.13, 14.15, -1.5,
        23.54, 12.9, 44.56, -1.0, 2.3, 0.0, 4.4, 5.5 } } },

      { { { 1.2, 3.4, -5.6, 7.2, 10.11, -12.13, -14.5, -1.5,
        2.56, 12.89, 23.56, -1.0, 112.3, 0.0, 4.4, 2.5 } } },

      { { { 1.2, 31.41, 58.6, 7.2, 10.11, -12.13, -14.5, -1.5,
        2.56, 12.89, 11.56, -1.0, 112.3, 0.0, 44.4, 2.5 } } },

      { { { 1.20, 3.4, -5.6, 7.0, 10.11, -12.156, -14.5, -1.5,
        2.56, 1.829, 23.6, -1.0, 112.3, 0.0, 41.4, 2.5 } } },
};

static HRESULT transform_setup_handler(
    state_test* test) {

    transform_context *ctx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(transform_context));
    if (ctx == NULL) return E_FAIL;
    test->test_context = ctx;

    test->return_data = &ctx->return_data_buffer;
    test->test_data_in = &transform_test_data;
    test->test_data_out = &transform_test_data;
    test->default_data = &transform_default_data;
    test->initial_data = &transform_default_data;
    test->poison_data = &transform_poison_data;

    test->data_size = sizeof(transform_data);

    return D3D_OK;
}

static void transform_teardown_handler(
    state_test* test) {
    
    HeapFree(GetProcessHeap(), 0, test->test_context);
}

static void transform_queue_test(
    state_test* test) {

    test->setup_handler = transform_setup_handler;
    test->teardown_handler = transform_teardown_handler;
    test->set_handler = transform_set_handler;
    test->get_handler = transform_get_handler;
    test->print_handler = transform_print_handler;
    test->test_name = "set_get_transforms";
    test->test_arg = NULL;
}

/* =================== State test: Render States ===================================== */

#define D3D9_RENDER_STATES 102
const D3DRENDERSTATETYPE render_state_indices[] = {

    D3DRS_ZENABLE,
    D3DRS_FILLMODE,
    D3DRS_SHADEMODE,
    D3DRS_ZWRITEENABLE,
    D3DRS_ALPHATESTENABLE,
    D3DRS_LASTPIXEL,
    D3DRS_SRCBLEND,
    D3DRS_DESTBLEND,
    D3DRS_CULLMODE,
    D3DRS_ZFUNC,
    D3DRS_ALPHAREF,
    D3DRS_ALPHAFUNC,
    D3DRS_DITHERENABLE,
    D3DRS_ALPHABLENDENABLE,
    D3DRS_FOGENABLE,
    D3DRS_SPECULARENABLE,
    D3DRS_FOGCOLOR,
    D3DRS_FOGTABLEMODE,
    D3DRS_FOGSTART,
    D3DRS_FOGEND,
    D3DRS_FOGDENSITY,
    D3DRS_RANGEFOGENABLE,
    D3DRS_STENCILENABLE,
    D3DRS_STENCILFAIL,
    D3DRS_STENCILZFAIL,
    D3DRS_STENCILPASS,
    D3DRS_STENCILFUNC,
    D3DRS_STENCILREF,
    D3DRS_STENCILMASK,
    D3DRS_STENCILWRITEMASK,
    D3DRS_TEXTUREFACTOR,
    D3DRS_WRAP0,
    D3DRS_WRAP1,
    D3DRS_WRAP2,
    D3DRS_WRAP3,
    D3DRS_WRAP4,
    D3DRS_WRAP5,
    D3DRS_WRAP6,
    D3DRS_WRAP7,
    D3DRS_CLIPPING,
    D3DRS_LIGHTING,
    D3DRS_AMBIENT,
    D3DRS_FOGVERTEXMODE,
    D3DRS_COLORVERTEX,
    D3DRS_LOCALVIEWER,
    D3DRS_NORMALIZENORMALS,
    D3DRS_DIFFUSEMATERIALSOURCE,
    D3DRS_SPECULARMATERIALSOURCE,
    D3DRS_AMBIENTMATERIALSOURCE,
    D3DRS_EMISSIVEMATERIALSOURCE,
    D3DRS_VERTEXBLEND,
    D3DRS_CLIPPLANEENABLE,
#if 0 /* Driver dependent, increase array size to enable */
    D3DRS_POINTSIZE,
#endif
    D3DRS_POINTSIZE_MIN,
    D3DRS_POINTSPRITEENABLE,
    D3DRS_POINTSCALEENABLE,
    D3DRS_POINTSCALE_A,
    D3DRS_POINTSCALE_B,
    D3DRS_POINTSCALE_C,
    D3DRS_MULTISAMPLEANTIALIAS,
    D3DRS_MULTISAMPLEMASK,
    D3DRS_PATCHEDGESTYLE,
    D3DRS_DEBUGMONITORTOKEN,
    D3DRS_POINTSIZE_MAX,
    D3DRS_INDEXEDVERTEXBLENDENABLE,
    D3DRS_COLORWRITEENABLE,
    D3DRS_TWEENFACTOR,
    D3DRS_BLENDOP,
    D3DRS_POSITIONDEGREE,
    D3DRS_NORMALDEGREE,
    D3DRS_SCISSORTESTENABLE,
    D3DRS_SLOPESCALEDEPTHBIAS,
    D3DRS_ANTIALIASEDLINEENABLE,
    D3DRS_MINTESSELLATIONLEVEL,
    D3DRS_MAXTESSELLATIONLEVEL,
    D3DRS_ADAPTIVETESS_X,
    D3DRS_ADAPTIVETESS_Y,
    D3DRS_ADAPTIVETESS_Z,
    D3DRS_ADAPTIVETESS_W,
    D3DRS_ENABLEADAPTIVETESSELLATION,
    D3DRS_TWOSIDEDSTENCILMODE,
    D3DRS_CCW_STENCILFAIL,
    D3DRS_CCW_STENCILZFAIL,
    D3DRS_CCW_STENCILPASS,
    D3DRS_CCW_STENCILFUNC,
    D3DRS_COLORWRITEENABLE1,
    D3DRS_COLORWRITEENABLE2,
    D3DRS_COLORWRITEENABLE3,
    D3DRS_BLENDFACTOR,
    D3DRS_SRGBWRITEENABLE,
    D3DRS_DEPTHBIAS,
    D3DRS_WRAP8,
    D3DRS_WRAP9,
    D3DRS_WRAP10,
    D3DRS_WRAP11,
    D3DRS_WRAP12,
    D3DRS_WRAP13,
    D3DRS_WRAP14,
    D3DRS_WRAP15,
    D3DRS_SEPARATEALPHABLENDENABLE,
    D3DRS_SRCBLENDALPHA,
    D3DRS_DESTBLENDALPHA,
    D3DRS_BLENDOPALPHA,
};

typedef struct render_state_data {
    DWORD states[D3D9_RENDER_STATES];
} render_state_data;

typedef struct render_state_arg {
    D3DPRESENT_PARAMETERS* device_pparams;
} render_state_arg;

typedef struct render_state_context {
   render_state_data return_data_buffer;
   render_state_data default_data_buffer;
   render_state_data test_data_buffer;
   render_state_data poison_data_buffer;
} render_state_context;

static void render_state_set_handler(
    IDirect3DDevice9* device, const state_test* test, const void* data) {

    HRESULT hret;
    const render_state_data* rsdata = data;
    unsigned int i;

    for (i = 0; i < D3D9_RENDER_STATES; i++) {
        hret = IDirect3DDevice9_SetRenderState(device, render_state_indices[i], rsdata->states[i]);
        ok(hret == D3D_OK, "SetRenderState returned %#x.\n", hret);
    }
}

static void render_state_get_handler(
    IDirect3DDevice9* device, const state_test* test, void* data) {

    HRESULT hret;
    render_state_data* rsdata = (render_state_data*) data;
    unsigned int i = 0;

    for (i = 0; i < D3D9_RENDER_STATES; i++) {
        hret = IDirect3DDevice9_GetRenderState(device, render_state_indices[i], &rsdata->states[i]);
        ok(hret == D3D_OK, "GetRenderState returned %#x.\n", hret);
    }
}

static void render_state_print_handler(
    const state_test* test,
    const void* data) {

    const render_state_data* rsdata = data;

    unsigned int i;
    for (i = 0; i < D3D9_RENDER_STATES; i++)
        trace("Index = %u, Value = %#x\n", i, rsdata->states[i]);
}

static inline DWORD to_dword(float fl) {
    return *((DWORD*) &fl);
}

static void render_state_default_data_init(
    D3DPRESENT_PARAMETERS* device_pparams,
    render_state_data* data) {

   unsigned int idx = 0;

   data->states[idx++] = device_pparams->EnableAutoDepthStencil?
                         D3DZB_TRUE : D3DZB_FALSE;  /* ZENABLE */
   data->states[idx++] = D3DFILL_SOLID;         /* FILLMODE */
   data->states[idx++] = D3DSHADE_GOURAUD;      /* SHADEMODE */
   data->states[idx++] = TRUE;                  /* ZWRITEENABLE */
   data->states[idx++] = FALSE;                 /* ALPHATESTENABLE */
   data->states[idx++] = TRUE;                  /* LASTPIXEL */
   data->states[idx++] = D3DBLEND_ONE;          /* SRCBLEND */
   data->states[idx++] = D3DBLEND_ZERO;         /* DESTBLEND */
   data->states[idx++] = D3DCULL_CCW;           /* CULLMODE */
   data->states[idx++] = D3DCMP_LESSEQUAL;      /* ZFUNC */
   data->states[idx++] = 0;                     /* ALPHAREF */
   data->states[idx++] = D3DCMP_ALWAYS;         /* ALPHAFUNC */
   data->states[idx++] = FALSE;                 /* DITHERENABLE */
   data->states[idx++] = FALSE;                 /* ALPHABLENDENABLE */
   data->states[idx++] = FALSE;                 /* FOGENABLE */
   data->states[idx++] = FALSE;                 /* SPECULARENABLE */
   data->states[idx++] = 0;                     /* FOGCOLOR */
   data->states[idx++] = D3DFOG_NONE;           /* FOGTABLEMODE */
   data->states[idx++] = to_dword(0.0f);        /* FOGSTART */
   data->states[idx++] = to_dword(1.0f);        /* FOGEND */
   data->states[idx++] = to_dword(1.0f);        /* FOGDENSITY */
   data->states[idx++] = FALSE;                 /* RANGEFOGENABLE */
   data->states[idx++] = FALSE;                 /* STENCILENABLE */
   data->states[idx++] = D3DSTENCILOP_KEEP;     /* STENCILFAIL */
   data->states[idx++] = D3DSTENCILOP_KEEP;     /* STENCILZFAIL */
   data->states[idx++] = D3DSTENCILOP_KEEP;     /* STENCILPASS */
   data->states[idx++] = D3DCMP_ALWAYS;         /* STENCILFUNC */
   data->states[idx++] = 0;                     /* STENCILREF */
   data->states[idx++] = 0xFFFFFFFF;            /* STENCILMASK */
   data->states[idx++] = 0xFFFFFFFF;            /* STENCILWRITEMASK */
   data->states[idx++] = 0xFFFFFFFF;            /* TEXTUREFACTOR */
   data->states[idx++] = 0;                     /* WRAP 0 */
   data->states[idx++] = 0;                     /* WRAP 1 */
   data->states[idx++] = 0;                     /* WRAP 2 */
   data->states[idx++] = 0;                     /* WRAP 3 */
   data->states[idx++] = 0;                     /* WRAP 4 */
   data->states[idx++] = 0;                     /* WRAP 5 */
   data->states[idx++] = 0;                     /* WRAP 6 */
   data->states[idx++] = 0;                     /* WRAP 7 */
   data->states[idx++] = TRUE;                  /* CLIPPING */
   data->states[idx++] = TRUE;                  /* LIGHTING */
   data->states[idx++] = 0;                     /* AMBIENT */
   data->states[idx++] = D3DFOG_NONE;           /* FOGVERTEXMODE */
   data->states[idx++] = TRUE;                  /* COLORVERTEX */
   data->states[idx++] = TRUE;                  /* LOCALVIEWER */
   data->states[idx++] = FALSE;                 /* NORMALIZENORMALS */
   data->states[idx++] = D3DMCS_COLOR1;         /* DIFFUSEMATERIALSOURCE */
   data->states[idx++] = D3DMCS_COLOR2;         /* SPECULARMATERIALSOURCE */
   data->states[idx++] = D3DMCS_MATERIAL;       /* AMBIENTMATERIALSOURCE */
   data->states[idx++] = D3DMCS_MATERIAL;       /* EMISSIVEMATERIALSOURCE */
   data->states[idx++] = D3DVBF_DISABLE;        /* VERTEXBLEND */
   data->states[idx++] = 0;                     /* CLIPPLANEENABLE */
#if 0 /* Driver dependent, increase array size to enable */
   data->states[idx++] = to_dword(1.0f);       /* POINTSIZE */
#endif
   data->states[idx++] = to_dword(1.0f);        /* POINTSIZEMIN */
   data->states[idx++] = FALSE;                 /* POINTSPRITEENABLE */
   data->states[idx++] = FALSE;                 /* POINTSCALEENABLE */
   data->states[idx++] = to_dword(1.0f);        /* POINTSCALE_A */
   data->states[idx++] = to_dword(0.0f);        /* POINTSCALE_B */
   data->states[idx++] = to_dword(0.0f);        /* POINTSCALE_C */
   data->states[idx++] = TRUE;                  /* MULTISAMPLEANTIALIAS */
   data->states[idx++] = 0xFFFFFFFF;            /* MULTISAMPLEMASK */
   data->states[idx++] = D3DPATCHEDGE_DISCRETE; /* PATCHEDGESTYLE */ 
   data->states[idx++] = 0xbaadcafe;            /* DEBUGMONITORTOKEN */
   data->states[idx++] = to_dword(64.0f);       /* POINTSIZE_MAX */
   data->states[idx++] = FALSE;                 /* INDEXEDVERTEXBLENDENABLE */
   data->states[idx++] = 0x0000000F;            /* COLORWRITEENABLE */
   data->states[idx++] = to_dword(0.0f);        /* TWEENFACTOR */
   data->states[idx++] = D3DBLENDOP_ADD;        /* BLENDOP */
   data->states[idx++] = D3DDEGREE_CUBIC;       /* POSITIONDEGREE */
   data->states[idx++] = D3DDEGREE_LINEAR;      /* NORMALDEGREE */
   data->states[idx++] = FALSE;                 /* SCISSORTESTENABLE */
   data->states[idx++] = to_dword(0.0f);        /* SLOPESCALEDEPTHBIAS */
   data->states[idx++] = FALSE;                 /* ANTIALIASEDLINEENABLE */
   data->states[idx++] = to_dword(1.0f);        /* MINTESSELATIONLEVEL */
   data->states[idx++] = to_dword(1.0f);        /* MAXTESSELATIONLEVEL */
   data->states[idx++] = to_dword(0.0f);        /* ADAPTIVETESS_X */
   data->states[idx++] = to_dword(0.0f);        /* ADAPTIVETESS_Y */
   data->states[idx++] = to_dword(1.0f);        /* ADAPTIVETESS_Z */
   data->states[idx++] = to_dword(0.0f);        /* ADAPTIVETESS_W */
   data->states[idx++] = FALSE;                 /* ENABLEADAPTIVETESSELATION */
   data->states[idx++] = FALSE;                 /* TWOSIDEDSTENCILMODE */
   data->states[idx++] = D3DSTENCILOP_KEEP;     /* CCW_STENCILFAIL */
   data->states[idx++] = D3DSTENCILOP_KEEP;     /* CCW_STENCILZFAIL */
   data->states[idx++] = D3DSTENCILOP_KEEP;     /* CCW_STENCILPASS */
   data->states[idx++] = D3DCMP_ALWAYS;         /* CCW_STENCILFUNC */
   data->states[idx++] = 0x0000000F;            /* COLORWRITEENABLE1 */
   data->states[idx++] = 0x0000000F;            /* COLORWRITEENABLE2 */
   data->states[idx++] = 0x0000000F;            /* COLORWRITEENABLE3 */
   data->states[idx++] = 0xFFFFFFFF;            /* BLENDFACTOR */
   data->states[idx++] = 0;                     /* SRGBWRITEENABLE */
   data->states[idx++] = to_dword(0.0f);        /* DEPTHBIAS */
   data->states[idx++] = 0;                     /* WRAP8 */
   data->states[idx++] = 0;                     /* WRAP9 */
   data->states[idx++] = 0;                     /* WRAP10 */
   data->states[idx++] = 0;                     /* WRAP11 */
   data->states[idx++] = 0;                     /* WRAP12 */
   data->states[idx++] = 0;                     /* WRAP13 */
   data->states[idx++] = 0;                     /* WRAP14 */
   data->states[idx++] = 0;                     /* WRAP15 */
   data->states[idx++] = FALSE;                 /* SEPARATEALPHABLENDENABLE */
   data->states[idx++] = D3DBLEND_ONE;          /* SRCBLENDALPHA */
   data->states[idx++] = D3DBLEND_ZERO;         /* DESTBLENDALPHA */
   data->states[idx++] = TRUE;                  /* BLENDOPALPHA */
}

static void render_state_poison_data_init(
    render_state_data* data) {

   unsigned int i;
   for (i = 0; i < D3D9_RENDER_STATES; i++)
       data->states[i] = 0x1337c0de;
}
 
static void render_state_test_data_init(
    render_state_data* data) {

   unsigned int idx = 0;
   data->states[idx++] = D3DZB_USEW;            /* ZENABLE */
   data->states[idx++] = D3DFILL_WIREFRAME;     /* FILLMODE */
   data->states[idx++] = D3DSHADE_PHONG;        /* SHADEMODE */
   data->states[idx++] = FALSE;                 /* ZWRITEENABLE */
   data->states[idx++] = TRUE;                  /* ALPHATESTENABLE */
   data->states[idx++] = FALSE;                 /* LASTPIXEL */
   data->states[idx++] = D3DBLEND_SRCALPHASAT;  /* SRCBLEND */
   data->states[idx++] = D3DBLEND_INVDESTALPHA; /* DESTBLEND */
   data->states[idx++] = D3DCULL_CW;            /* CULLMODE */
   data->states[idx++] = D3DCMP_NOTEQUAL;       /* ZFUNC */
   data->states[idx++] = 10;                    /* ALPHAREF */
   data->states[idx++] = D3DCMP_GREATER;        /* ALPHAFUNC */
   data->states[idx++] = TRUE;                  /* DITHERENABLE */
   data->states[idx++] = TRUE;                  /* ALPHABLENDENABLE */
   data->states[idx++] = TRUE;                  /* FOGENABLE */
   data->states[idx++] = TRUE;                  /* SPECULARENABLE */
   data->states[idx++] = 255 << 31;             /* FOGCOLOR */
   data->states[idx++] = D3DFOG_EXP;            /* FOGTABLEMODE */
   data->states[idx++] = to_dword(0.1f);        /* FOGSTART */
   data->states[idx++] = to_dword(0.8f);        /* FOGEND */
   data->states[idx++] = to_dword(0.5f);        /* FOGDENSITY */
   data->states[idx++] = TRUE;                  /* RANGEFOGENABLE */
   data->states[idx++] = TRUE;                  /* STENCILENABLE */
   data->states[idx++] = D3DSTENCILOP_INCRSAT;  /* STENCILFAIL */
   data->states[idx++] = D3DSTENCILOP_REPLACE;  /* STENCILZFAIL */
   data->states[idx++] = D3DSTENCILOP_INVERT;   /* STENCILPASS */
   data->states[idx++] = D3DCMP_LESS;           /* STENCILFUNC */
   data->states[idx++] = 10;                    /* STENCILREF */
   data->states[idx++] = 0xFF00FF00;            /* STENCILMASK */
   data->states[idx++] = 0x00FF00FF;            /* STENCILWRITEMASK */
   data->states[idx++] = 0xF0F0F0F0;            /* TEXTUREFACTOR */
   data->states[idx++] = D3DWRAPCOORD_0 | D3DWRAPCOORD_2;                                   /* WRAP 0 */
   data->states[idx++] = D3DWRAPCOORD_1 | D3DWRAPCOORD_3;                                   /* WRAP 1 */
   data->states[idx++] = D3DWRAPCOORD_2 | D3DWRAPCOORD_3;                                   /* WRAP 2 */
   data->states[idx++] = D3DWRAPCOORD_3 | D3DWRAPCOORD_0;                                   /* WRAP 4 */
   data->states[idx++] = D3DWRAPCOORD_0 | D3DWRAPCOORD_1 | D3DWRAPCOORD_2;                  /* WRAP 5 */
   data->states[idx++] = D3DWRAPCOORD_1 | D3DWRAPCOORD_3 | D3DWRAPCOORD_2;                  /* WRAP 6 */
   data->states[idx++] = D3DWRAPCOORD_2 | D3DWRAPCOORD_1 | D3DWRAPCOORD_0;                  /* WRAP 7 */
   data->states[idx++] = D3DWRAPCOORD_1 | D3DWRAPCOORD_0 | D3DWRAPCOORD_2 | D3DWRAPCOORD_3; /* WRAP 8 */
   data->states[idx++] = FALSE;                 /* CLIPPING */
   data->states[idx++] = FALSE;                 /* LIGHTING */
   data->states[idx++] = 255 << 16;             /* AMBIENT */
   data->states[idx++] = D3DFOG_EXP2;           /* FOGVERTEXMODE */
   data->states[idx++] = FALSE;                 /* COLORVERTEX */
   data->states[idx++] = FALSE;                 /* LOCALVIEWER */
   data->states[idx++] = TRUE;                  /* NORMALIZENORMALS */
   data->states[idx++] = D3DMCS_COLOR2;         /* DIFFUSEMATERIALSOURCE */
   data->states[idx++] = D3DMCS_MATERIAL;       /* SPECULARMATERIALSOURCE */
   data->states[idx++] = D3DMCS_COLOR1;         /* AMBIENTMATERIALSOURCE */
   data->states[idx++] = D3DMCS_COLOR2;         /* EMISSIVEMATERIALSOURCE */
   data->states[idx++] = D3DVBF_3WEIGHTS;       /* VERTEXBLEND */
   data->states[idx++] = 0xf1f1f1f1;            /* CLIPPLANEENABLE */
#if 0 /* Driver dependent, increase array size to enable */
   data->states[idx++] = to_dword(32.0f);       /* POINTSIZE */
#endif
   data->states[idx++] = to_dword(0.7f);        /* POINTSIZEMIN */
   data->states[idx++] = TRUE;                  /* POINTSPRITEENABLE */
   data->states[idx++] = TRUE;                  /* POINTSCALEENABLE */
   data->states[idx++] = to_dword(0.7f);        /* POINTSCALE_A */
   data->states[idx++] = to_dword(0.5f);        /* POINTSCALE_B */
   data->states[idx++] = to_dword(0.4f);        /* POINTSCALE_C */
   data->states[idx++] = FALSE;                 /* MULTISAMPLEANTIALIAS */
   data->states[idx++] = 0xABCDDBCA;            /* MULTISAMPLEMASK */
   data->states[idx++] = D3DPATCHEDGE_CONTINUOUS; /* PATCHEDGESTYLE */
   data->states[idx++] = D3DDMT_DISABLE;        /* DEBUGMONITORTOKEN */
   data->states[idx++] = to_dword(77.0f);       /* POINTSIZE_MAX */
   data->states[idx++] = TRUE;                  /* INDEXEDVERTEXBLENDENABLE */
   data->states[idx++] = 0x00000009;            /* COLORWRITEENABLE */
   data->states[idx++] = to_dword(0.2f);        /* TWEENFACTOR */
   data->states[idx++] = D3DBLENDOP_REVSUBTRACT;/* BLENDOP */
   data->states[idx++] = D3DDEGREE_LINEAR;      /* POSITIONDEGREE */
   data->states[idx++] = D3DDEGREE_CUBIC;       /* NORMALDEGREE */
   data->states[idx++] = TRUE;                  /* SCISSORTESTENABLE */
   data->states[idx++] = to_dword(0.33f);       /* SLOPESCALEDEPTHBIAS */
   data->states[idx++] = TRUE;                  /* ANTIALIASEDLINEENABLE */
   data->states[idx++] = to_dword(0.8f);        /* MINTESSELATIONLEVEL */
   data->states[idx++] = to_dword(0.8f);        /* MAXTESSELATIONLEVEL */
   data->states[idx++] = to_dword(0.2f);        /* ADAPTIVETESS_X */
   data->states[idx++] = to_dword(0.3f);        /* ADAPTIVETESS_Y */
   data->states[idx++] = to_dword(0.6f);        /* ADAPTIVETESS_Z */
   data->states[idx++] = to_dword(0.4f);        /* ADAPTIVETESS_W */
   data->states[idx++] = TRUE;                  /* ENABLEADAPTIVETESSELATION */
   data->states[idx++] = TRUE;                  /* TWOSIDEDSTENCILMODE */
   data->states[idx++] = D3DSTENCILOP_ZERO;     /* CCW_STENCILFAIL */
   data->states[idx++] = D3DSTENCILOP_DECR;     /* CCW_STENCILZFAIL */
   data->states[idx++] = D3DSTENCILOP_INCR;     /* CCW_STENCILPASS */
   data->states[idx++] = D3DCMP_ALWAYS;         /* CCW_STENCILFUNC */
   data->states[idx++] = 0x00000007;            /* COLORWRITEENABLE1 */
   data->states[idx++] = 0x00000008;            /* COLORWRITEENABLE2 */
   data->states[idx++] = 0x00000004;            /* COLORWRITEENABLE3 */
   data->states[idx++] = 0xF0F1F2F3;            /* BLENDFACTOR */
   data->states[idx++] = 1;                     /* SRGBWRITEENABLE */
   data->states[idx++] = to_dword(0.3f);        /* DEPTHBIAS */
   data->states[idx++] = D3DWRAPCOORD_0 | D3DWRAPCOORD_2;                                   /* WRAP 8 */
   data->states[idx++] = D3DWRAPCOORD_1 | D3DWRAPCOORD_3;                                   /* WRAP 9 */
   data->states[idx++] = D3DWRAPCOORD_2 | D3DWRAPCOORD_3;                                   /* WRAP 10 */
   data->states[idx++] = D3DWRAPCOORD_3 | D3DWRAPCOORD_0;                                   /* WRAP 11 */
   data->states[idx++] = D3DWRAPCOORD_0 | D3DWRAPCOORD_1 | D3DWRAPCOORD_2;                  /* WRAP 12 */
   data->states[idx++] = D3DWRAPCOORD_1 | D3DWRAPCOORD_3 | D3DWRAPCOORD_2;                  /* WRAP 13 */
   data->states[idx++] = D3DWRAPCOORD_2 | D3DWRAPCOORD_1 | D3DWRAPCOORD_0;                  /* WRAP 14 */
   data->states[idx++] = D3DWRAPCOORD_1 | D3DWRAPCOORD_0 | D3DWRAPCOORD_2 | D3DWRAPCOORD_3; /* WRAP 15 */
   data->states[idx++] = TRUE;                  /* SEPARATEALPHABLENDENABLE */
   data->states[idx++] = D3DBLEND_ZERO;         /* SRCBLENDALPHA */
   data->states[idx++] = D3DBLEND_ONE;          /* DESTBLENDALPHA */
   data->states[idx++] = FALSE;                 /* BLENDOPALPHA */
}

static HRESULT render_state_setup_handler(
    state_test* test) {

    const render_state_arg* rsarg = test->test_arg;

    render_state_context *ctx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(render_state_context));
    if (ctx == NULL) return E_FAIL;
    test->test_context = ctx;

    test->return_data = &ctx->return_data_buffer;
    test->default_data = &ctx->default_data_buffer;
    test->initial_data = &ctx->default_data_buffer;
    test->test_data_in = &ctx->test_data_buffer;
    test->test_data_out = &ctx->test_data_buffer;
    test->poison_data = &ctx->poison_data_buffer;

    render_state_default_data_init(rsarg->device_pparams, &ctx->default_data_buffer);
    render_state_test_data_init(&ctx->test_data_buffer);
    render_state_poison_data_init(&ctx->poison_data_buffer);

    test->data_size = sizeof(render_state_data);

    return D3D_OK;
}

static void render_state_teardown_handler(
    state_test* test) {

    HeapFree(GetProcessHeap(), 0, test->test_context);
}

static void render_states_queue_test(
    state_test* test,
    const render_state_arg* test_arg) {

    test->setup_handler = render_state_setup_handler;
    test->teardown_handler = render_state_teardown_handler;
    test->set_handler = render_state_set_handler;
    test->get_handler = render_state_get_handler;
    test->print_handler = render_state_print_handler;
    test->test_name = "set_get_render_states";
    test->test_arg = test_arg;
}

/* =================== Main state tests function =============================== */

static void test_state_management(
    IDirect3DDevice9 *device,
    D3DPRESENT_PARAMETERS *device_pparams) {

    HRESULT hret;
    D3DCAPS9 caps;

    /* Test count: 2 for shader constants 
                   1 for lights
                   1 for transforms
                   1 for render states
     */
    const int max_tests = 5;
    state_test tests[5];
    unsigned int tcount = 0;

    shader_constant_arg pshader_constant_arg;
    shader_constant_arg vshader_constant_arg;
    render_state_arg render_state_arg;
    light_arg light_arg;

    hret = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hret == D3D_OK, "GetDeviceCaps returned %#x.\n", hret);
    if (hret != D3D_OK) return;

    texture_stages = caps.MaxTextureBlendStages;

    /* Zero test memory */
    memset(tests, 0, sizeof(state_test) * max_tests);

    if (caps.VertexShaderVersion & 0xffff) {
        vshader_constant_arg.idx = 0;
        vshader_constant_arg.pshader = FALSE;
        shader_constants_queue_test(&tests[tcount], &vshader_constant_arg);
        tcount++;
    }

    if (caps.PixelShaderVersion & 0xffff) {
        pshader_constant_arg.idx = 0;
        pshader_constant_arg.pshader = TRUE;
        shader_constants_queue_test(&tests[tcount], &pshader_constant_arg);
        tcount++;
    }

    light_arg.idx = 0;
    lights_queue_test(&tests[tcount], &light_arg);
    tcount++;

    transform_queue_test(&tests[tcount]);
    tcount++;

    render_state_arg.device_pparams = device_pparams;
    render_states_queue_test(&tests[tcount], &render_state_arg);
    tcount++;

    execute_test_chain_all(device, tests, tcount);
}

START_TEST(stateblock)
{
    IDirect3DDevice9 *device_ptr = NULL;
    D3DPRESENT_PARAMETERS device_pparams;
    HRESULT hret;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    hret = init_d3d9(&device_ptr, &device_pparams);
    if (hret != D3D_OK) return;

    test_begin_end_state_block(device_ptr);
    test_state_management(device_ptr, &device_pparams);

    if (device_ptr) IUnknown_Release(device_ptr);
}
