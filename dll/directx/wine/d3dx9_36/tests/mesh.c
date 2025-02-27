/*
 * Copyright 2008 David Adam
 * Copyright 2008 Luis Busquets
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2011 Michael Mc Donnell
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
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <assert.h>
#include "wine/test.h"
#include "d3dx9.h"
#include "initguid.h"
#include "rmxftmpl.h"
#include "rmxfguid.h"

/* Set the WINETEST_DEBUG environment variable to be greater than 1 for verbose
 * function call traces of ID3DXAllocateHierarchy callbacks. */
#define TRACECALLBACK if(winetest_debug > 1) trace

#define admitted_error 0.0001f

#define compare_vertex_sizes(type, exp) \
    got=D3DXGetFVFVertexSize(type); \
    ok(got==exp, "Expected: %d, Got: %d\n", exp, got);

#define compare_float(got, exp) \
    do { \
        float _got = (got); \
        float _exp = (exp); \
        ok(_got == _exp, "Expected: %g, Got: %g\n", _exp, _got); \
    } while (0)

static BOOL compare(FLOAT u, FLOAT v)
{
    return (fabs(u-v) < admitted_error);
}

static BOOL compare_vec3(D3DXVECTOR3 u, D3DXVECTOR3 v)
{
    return ( compare(u.x, v.x) && compare(u.y, v.y) && compare(u.z, v.z) );
}

static BOOL compare_vec4(D3DXVECTOR4 u, D3DXVECTOR4 v)
{
    return compare(u.x, v.x) && compare(u.y, v.y) && compare(u.z, v.z) && compare(u.w, v.w);
}

#define check_floats(got, exp, dim) check_floats_(__LINE__, "", got, exp, dim)
static void check_floats_(int line, const char *prefix, const float *got, const float *exp, int dim)
{
    int i;
    char exp_buffer[256] = "";
    char got_buffer[256] = "";
    char *exp_buffer_ptr = exp_buffer;
    char *got_buffer_ptr = got_buffer;
    BOOL equal = TRUE;

    for (i = 0; i < dim; i++) {
        if (i) {
            exp_buffer_ptr += sprintf(exp_buffer_ptr, ", ");
            got_buffer_ptr += sprintf(got_buffer_ptr, ", ");
        }
        equal = equal && compare(*exp, *got);
        exp_buffer_ptr += sprintf(exp_buffer_ptr, "%g", *exp);
        got_buffer_ptr += sprintf(got_buffer_ptr, "%g", *got);
        exp++; got++;
    }
    ok_(__FILE__,line)(equal, "%sExpected (%s), got (%s)", prefix, exp_buffer, got_buffer);
}

struct vertex
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
};

typedef WORD face[3];

static BOOL compare_face(face a, face b)
{
    return (a[0]==b[0] && a[1] == b[1] && a[2] == b[2]);
}

struct test_context
{
    HWND hwnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
};

/* Initializes a test context struct. Use it to initialize DirectX.
 *
 * Returns NULL if an error occurred.
 */
static struct test_context *new_test_context(void)
{
    HRESULT hr;
    HWND hwnd = NULL;
    IDirect3D9 *d3d = NULL;
    IDirect3DDevice9 *device = NULL;
    D3DPRESENT_PARAMETERS d3dpp = {0};
    struct test_context *test_context;

    if (!(hwnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        goto error;
    }

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object\n");
        goto error;
    }

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                 D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Couldn't create IDirect3DDevice9 object %#lx\n", hr);
        goto error;
    }

    test_context = malloc(sizeof(*test_context));
    if (!test_context)
    {
        skip("Couldn't allocate memory for test_context\n");
        goto error;
    }
    test_context->hwnd = hwnd;
    test_context->d3d = d3d;
    test_context->device = device;

    return test_context;

error:
    if (device)
        IDirect3DDevice9_Release(device);

    if (d3d)
        IDirect3D9_Release(d3d);

    if (hwnd)
        DestroyWindow(hwnd);

    return NULL;
}

static void free_test_context(struct test_context *test_context)
{
    if (!test_context)
        return;

    if (test_context->device)
        IDirect3DDevice9_Release(test_context->device);

    if (test_context->d3d)
        IDirect3D9_Release(test_context->d3d);

    if (test_context->hwnd)
        DestroyWindow(test_context->hwnd);

    free(test_context);
}

struct mesh
{
    DWORD number_of_vertices;
    struct vertex *vertices;

    DWORD number_of_faces;
    face *faces;

    DWORD fvf;
    UINT vertex_size;
};

static void free_mesh(struct mesh *mesh)
{
    free(mesh->faces);
    free(mesh->vertices);
}

static BOOL new_mesh(struct mesh *mesh, DWORD number_of_vertices, DWORD number_of_faces)
{
    mesh->vertices = calloc(number_of_vertices, sizeof(*mesh->vertices));
    if (!mesh->vertices)
    {
        return FALSE;
    }
    mesh->number_of_vertices = number_of_vertices;

    mesh->faces = calloc(number_of_faces, sizeof(*mesh->faces));
    if (!mesh->faces)
    {
        free(mesh->vertices);
        return FALSE;
    }
    mesh->number_of_faces = number_of_faces;

    return TRUE;
}

static void compare_mesh(const char *name, ID3DXMesh *d3dxmesh, struct mesh *mesh)
{
    HRESULT hr;
    DWORD number_of_vertices, number_of_faces;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    D3DVERTEXBUFFER_DESC vertex_buffer_description;
    D3DINDEXBUFFER_DESC index_buffer_description;
    struct vertex *vertices;
    face *faces;
    int expected, i;

    number_of_vertices = d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh);
    ok(number_of_vertices == mesh->number_of_vertices, "Test %s, result %lu, expected %ld\n",
       name, number_of_vertices, mesh->number_of_vertices);

    number_of_faces = d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh);
    ok(number_of_faces == mesh->number_of_faces, "Test %s, result %lu, expected %ld\n",
       name, number_of_faces, mesh->number_of_faces);

    /* vertex buffer */
    hr = d3dxmesh->lpVtbl->GetVertexBuffer(d3dxmesh, &vertex_buffer);
    ok(hr == D3D_OK, "Test %s, result %lx, expected 0 (D3D_OK)\n", name, hr);

    if (hr != D3D_OK)
    {
        skip("Couldn't get vertex buffer\n");
    }
    else
    {
        hr = IDirect3DVertexBuffer9_GetDesc(vertex_buffer, &vertex_buffer_description);
        ok(hr == D3D_OK, "Test %s, result %lx, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't get vertex buffer description\n");
        }
        else
        {
            ok(vertex_buffer_description.Format == D3DFMT_VERTEXDATA, "Test %s, result %x, expected %x (D3DFMT_VERTEXDATA)\n",
               name, vertex_buffer_description.Format, D3DFMT_VERTEXDATA);
            ok(vertex_buffer_description.Type == D3DRTYPE_VERTEXBUFFER, "Test %s, result %x, expected %x (D3DRTYPE_VERTEXBUFFER)\n",
               name, vertex_buffer_description.Type, D3DRTYPE_VERTEXBUFFER);
            ok(vertex_buffer_description.Usage == 0, "Test %s, result %lx, expected %x\n", name, vertex_buffer_description.Usage, 0);
            ok(vertex_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_MANAGED)\n",
               name, vertex_buffer_description.Pool, D3DPOOL_MANAGED);
            ok(vertex_buffer_description.FVF == mesh->fvf, "Test %s, result %lx, expected %lx\n",
               name, vertex_buffer_description.FVF, mesh->fvf);
            if (mesh->fvf == 0)
            {
                expected = number_of_vertices * mesh->vertex_size;
            }
            else
            {
                expected = number_of_vertices * D3DXGetFVFVertexSize(mesh->fvf);
            }
            ok(vertex_buffer_description.Size == expected, "Test %s, result %x, expected %x\n",
               name, vertex_buffer_description.Size, expected);
        }

        /* specify offset and size to avoid potential overruns */
        hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, number_of_vertices * sizeof(D3DXVECTOR3) * 2,
                (void **)&vertices, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %s, result %lx, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't lock vertex buffer\n");
        }
        else
        {
            for (i = 0; i < number_of_vertices; i++)
            {
                ok(compare_vec3(vertices[i].position, mesh->vertices[i].position),
                   "Test %s, vertex position %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i,
                   vertices[i].position.x, vertices[i].position.y, vertices[i].position.z,
                   mesh->vertices[i].position.x, mesh->vertices[i].position.y, mesh->vertices[i].position.z);
                ok(compare_vec3(vertices[i].normal, mesh->vertices[i].normal),
                   "Test %s, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i,
                   vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z,
                   mesh->vertices[i].normal.x, mesh->vertices[i].normal.y, mesh->vertices[i].normal.z);
            }

            IDirect3DVertexBuffer9_Unlock(vertex_buffer);
        }

        IDirect3DVertexBuffer9_Release(vertex_buffer);
    }

    /* index buffer */
    hr = d3dxmesh->lpVtbl->GetIndexBuffer(d3dxmesh, &index_buffer);
    ok(hr == D3D_OK, "Test %s, result %lx, expected 0 (D3D_OK)\n", name, hr);

    if (!index_buffer)
    {
        skip("Couldn't get index buffer\n");
    }
    else
    {
        hr = IDirect3DIndexBuffer9_GetDesc(index_buffer, &index_buffer_description);
        ok(hr == D3D_OK, "Test %s, result %lx, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't get index buffer description\n");
        }
        else
        {
            ok(index_buffer_description.Format == D3DFMT_INDEX16, "Test %s, result %x, expected %x (D3DFMT_INDEX16)\n",
               name, index_buffer_description.Format, D3DFMT_INDEX16);
            ok(index_buffer_description.Type == D3DRTYPE_INDEXBUFFER, "Test %s, result %x, expected %x (D3DRTYPE_INDEXBUFFER)\n",
               name, index_buffer_description.Type, D3DRTYPE_INDEXBUFFER);
            ok(index_buffer_description.Usage == 0, "Test %s, result %#lx, expected %#x.\n",
                    name, index_buffer_description.Usage, 0);
            ok(index_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_MANAGED)\n",
               name, index_buffer_description.Pool, D3DPOOL_MANAGED);
            expected = number_of_faces * sizeof(WORD) * 3;
            ok(index_buffer_description.Size == expected, "Test %s, result %x, expected %x\n",
               name, index_buffer_description.Size, expected);
        }

        /* specify offset and size to avoid potential overruns */
        hr = IDirect3DIndexBuffer9_Lock(index_buffer, 0, number_of_faces * sizeof(WORD) * 3,
                (void **)&faces, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %s, result %lx, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't lock index buffer\n");
        }
        else
        {
            for (i = 0; i < number_of_faces; i++)
            {
                ok(compare_face(faces[i], mesh->faces[i]),
                   "Test %s, face %d, result (%u, %u, %u), expected (%u, %u, %u)\n", name, i,
                   faces[i][0], faces[i][1], faces[i][2],
                   mesh->faces[i][0], mesh->faces[i][1], mesh->faces[i][2]);
            }

            IDirect3DIndexBuffer9_Unlock(index_buffer);
        }

        IDirect3DIndexBuffer9_Release(index_buffer);
    }
}

static void D3DXBoundProbeTest(void)
{
    BOOL result;
    D3DXVECTOR3 bottom_point, center, top_point, raydirection, rayposition;
    FLOAT radius;

/*____________Test the Box case___________________________*/
    bottom_point.x = -3.0f; bottom_point.y = -2.0f; bottom_point.z = -1.0f;
    top_point.x = 7.0f; top_point.y = 8.0f; top_point.z = 9.0f;

    raydirection.x = -4.0f; raydirection.y = -5.0f; raydirection.z = -6.0f;
    rayposition.x = 5.0f; rayposition.y = 5.0f; rayposition.z = 11.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

    raydirection.x = 4.0f; raydirection.y = 5.0f; raydirection.z = 6.0f;
    rayposition.x = 5.0f; rayposition.y = 5.0f; rayposition.z = 11.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    rayposition.x = -4.0f; rayposition.y = 1.0f; rayposition.z = -2.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

    bottom_point.x = 1.0f; bottom_point.y = 0.0f; bottom_point.z = 0.0f;
    top_point.x = 1.0f; top_point.y = 0.0f; top_point.z = 0.0f;
    rayposition.x = 0.0f; rayposition.y = 1.0f; rayposition.z = 0.0f;
    raydirection.x = 0.0f; raydirection.y = 3.0f; raydirection.z = 0.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    bottom_point.x = 1.0f; bottom_point.y = 2.0f; bottom_point.z = 3.0f;
    top_point.x = 10.0f; top_point.y = 15.0f; top_point.z = 20.0f;

    raydirection.x = 7.0f; raydirection.y = 8.0f; raydirection.z = 9.0f;
    rayposition.x = 3.0f; rayposition.y = 7.0f; rayposition.z = -6.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

    bottom_point.x = 0.0f; bottom_point.y = 0.0f; bottom_point.z = 0.0f;
    top_point.x = 1.0f; top_point.y = 1.0f; top_point.z = 1.0f;

    raydirection.x = 0.0f; raydirection.y = 1.0f; raydirection.z = .0f;
    rayposition.x = -3.0f; rayposition.y = 0.0f; rayposition.z = 0.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    raydirection.x = 1.0f; raydirection.y = 0.0f; raydirection.z = .0f;
    rayposition.x = -3.0f; rayposition.y = 0.0f; rayposition.z = 0.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

/*____________Test the Sphere case________________________*/
    radius = sqrt(77.0f);
    center.x = 1.0f; center.y = 2.0f; center.z = 3.0f;
    raydirection.x = 2.0f; raydirection.y = -4.0f; raydirection.z = 2.0f;
    rayposition.x = 5.0f; rayposition.y = 5.0f; rayposition.z = 9.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == TRUE, "Got unexpected result %#x.\n", result);

    rayposition.x = 45.0f; rayposition.y = -75.0f; rayposition.z = 49.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "Got unexpected result %#x.\n", result);

    raydirection.x = -2.0f; raydirection.y = 4.0f; raydirection.z = -2.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == TRUE, "Got unexpected result %#x.\n", result);

    rayposition.x = 5.0f; rayposition.y = 11.0f; rayposition.z = 9.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "Got unexpected result %#x.\n", result);

    raydirection.x = 2.0f; raydirection.y = -4.0f; raydirection.z = 2.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "Got unexpected result %#x.\n", result);

    radius = 1.0f;
    rayposition.x = 2.0f; rayposition.y = 2.0f; rayposition.z = 3.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "Got unexpected result %#x.\n", result);

    raydirection.x = 0.0f; raydirection.y = 0.0f; raydirection.z = 1.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == TRUE, "Got unexpected result %#x.\n", result);

    if (0)
    {
        /* All these crash on native. */
        D3DXSphereBoundProbe(&center, radius, &rayposition, NULL);
        D3DXSphereBoundProbe(&center, radius, NULL, &raydirection);
        D3DXSphereBoundProbe(NULL, radius, &rayposition, &raydirection);
    }
}

static void D3DXComputeBoundingBoxTest(void)
{
    D3DXVECTOR3 exp_max, exp_min, got_max, got_min, vertex[5];
    HRESULT hr;

    vertex[0].x = 1.0f; vertex[0].y = 1.0f; vertex[0].z = 1.0f;
    vertex[1].x = 1.0f; vertex[1].y = 1.0f; vertex[1].z = 1.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 1.0f;
    vertex[3].x = 1.0f; vertex[3].y = 1.0f; vertex[3].z = 1.0f;
    vertex[4].x = 9.0f; vertex[4].y = 9.0f; vertex[4].z = 9.0f;

    exp_min.x = 1.0f; exp_min.y = 1.0f; exp_min.z = 1.0f;
    exp_max.x = 9.0f; exp_max.y = 9.0f; exp_max.z = 9.0f;

    hr = D3DXComputeBoundingBox(&vertex[3],2,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    ok( compare_vec3(exp_min,got_min), "Expected min: (%f, %f, %f), got: (%f, %f, %f)\n", exp_min.x,exp_min.y,exp_min.z,got_min.x,got_min.y,got_min.z);
    ok( compare_vec3(exp_max,got_max), "Expected max: (%f, %f, %f), got: (%f, %f, %f)\n", exp_max.x,exp_max.y,exp_max.z,got_max.x,got_max.y,got_max.z);

/*________________________*/

    vertex[0].x = 2.0f; vertex[0].y = 5.9f; vertex[0].z = -1.2f;
    vertex[1].x = -1.87f; vertex[1].y = 7.9f; vertex[1].z = 7.4f;
    vertex[2].x = 7.43f; vertex[2].y = -0.9f; vertex[2].z = 11.9f;
    vertex[3].x = -6.92f; vertex[3].y = 6.3f; vertex[3].z = -3.8f;
    vertex[4].x = 11.4f; vertex[4].y = -8.1f; vertex[4].z = 4.5f;

    exp_min.x = -6.92f; exp_min.y = -8.1f; exp_min.z = -3.80f;
    exp_max.x = 11.4f; exp_max.y = 7.90f; exp_max.z = 11.9f;

    hr = D3DXComputeBoundingBox(&vertex[0],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    ok( compare_vec3(exp_min,got_min), "Expected min: (%f, %f, %f), got: (%f, %f, %f)\n", exp_min.x,exp_min.y,exp_min.z,got_min.x,got_min.y,got_min.z);
    ok( compare_vec3(exp_max,got_max), "Expected max: (%f, %f, %f), got: (%f, %f, %f)\n", exp_max.x,exp_max.y,exp_max.z,got_max.x,got_max.y,got_max.z);

/*________________________*/

    vertex[0].x = 2.0f; vertex[0].y = 5.9f; vertex[0].z = -1.2f;
    vertex[1].x = -1.87f; vertex[1].y = 7.9f; vertex[1].z = 7.4f;
    vertex[2].x = 7.43f; vertex[2].y = -0.9f; vertex[2].z = 11.9f;
    vertex[3].x = -6.92f; vertex[3].y = 6.3f; vertex[3].z = -3.8f;
    vertex[4].x = 11.4f; vertex[4].y = -8.1f; vertex[4].z = 4.5f;

    exp_min.x = -6.92f; exp_min.y = -0.9f; exp_min.z = -3.8f;
    exp_max.x = 7.43f; exp_max.y = 7.90f; exp_max.z = 11.9f;

    hr = D3DXComputeBoundingBox(&vertex[0],4,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    ok( compare_vec3(exp_min,got_min), "Expected min: (%f, %f, %f), got: (%f, %f, %f)\n", exp_min.x,exp_min.y,exp_min.z,got_min.x,got_min.y,got_min.z);
    ok( compare_vec3(exp_max,got_max), "Expected max: (%f, %f, %f), got: (%f, %f, %f)\n", exp_max.x,exp_max.y,exp_max.z,got_max.x,got_max.y,got_max.z);

/*________________________*/
    hr = D3DXComputeBoundingBox(NULL,5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingBox(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),NULL,&got_max);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingBox(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,NULL);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);
}

static void D3DXComputeBoundingSphereTest(void)
{
    D3DXVECTOR3 exp_cen, got_cen, vertex[5];
    FLOAT exp_rad, got_rad;
    HRESULT hr;

    vertex[0].x = 1.0f; vertex[0].y = 1.0f; vertex[0].z = 1.0f;
    vertex[1].x = 1.0f; vertex[1].y = 1.0f; vertex[1].z = 1.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 1.0f;
    vertex[3].x = 1.0f; vertex[3].y = 1.0f; vertex[3].z = 1.0f;
    vertex[4].x = 9.0f; vertex[4].y = 9.0f; vertex[4].z = 9.0f;

    exp_rad = 6.928203f;
    exp_cen.x = 5.0; exp_cen.y = 5.0; exp_cen.z = 5.0;

    hr = D3DXComputeBoundingSphere(&vertex[3],2,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,&got_rad);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    ok( compare(exp_rad, got_rad), "Expected radius: %f, got radius: %f\n", exp_rad, got_rad);
    ok( compare_vec3(exp_cen,got_cen), "Expected center: (%f, %f, %f), got center: (%f, %f, %f)\n", exp_cen.x,exp_cen.y,exp_cen.z,got_cen.x,got_cen.y,got_cen.z);

/*________________________*/

    vertex[0].x = 2.0f; vertex[0].y = 5.9f; vertex[0].z = -1.2f;
    vertex[1].x = -1.87f; vertex[1].y = 7.9f; vertex[1].z = 7.4f;
    vertex[2].x = 7.43f; vertex[2].y = -0.9f; vertex[2].z = 11.9f;
    vertex[3].x = -6.92f; vertex[3].y = 6.3f; vertex[3].z = -3.8f;
    vertex[4].x = 11.4f; vertex[4].y = -8.1f; vertex[4].z = 4.5f;

    exp_rad = 13.707883f;
    exp_cen.x = 2.408f; exp_cen.y = 2.22f; exp_cen.z = 3.76f;

    hr = D3DXComputeBoundingSphere(&vertex[0],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,&got_rad);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    ok( compare(exp_rad, got_rad), "Expected radius: %f, got radius: %f\n", exp_rad, got_rad);
    ok( compare_vec3(exp_cen,got_cen), "Expected center: (%f, %f, %f), got center: (%f, %f, %f)\n", exp_cen.x,exp_cen.y,exp_cen.z,got_cen.x,got_cen.y,got_cen.z);

/*________________________*/
    hr = D3DXComputeBoundingSphere(NULL,5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,&got_rad);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingSphere(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),NULL,&got_rad);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingSphere(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,NULL);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);
}

static void print_elements(const D3DVERTEXELEMENT9 *elements)
{
    D3DVERTEXELEMENT9 last = D3DDECL_END();
    const D3DVERTEXELEMENT9 *ptr = elements;
    int count = 0;

    while (memcmp(ptr, &last, sizeof(D3DVERTEXELEMENT9)))
    {
        trace(
            "[Element %d] Stream = %d, Offset = %d, Type = %d, Method = %d, Usage = %d, UsageIndex = %d\n",
             count, ptr->Stream, ptr->Offset, ptr->Type, ptr->Method, ptr->Usage, ptr->UsageIndex);
        ptr++;
        count++;
    }
}

static void compare_elements(const D3DVERTEXELEMENT9 *elements, const D3DVERTEXELEMENT9 *expected_elements,
        unsigned int line, unsigned int test_id)
{
    D3DVERTEXELEMENT9 last = D3DDECL_END();
    unsigned int i;

    for (i = 0; i < MAX_FVF_DECL_SIZE; i++)
    {
        int end1 = memcmp(&elements[i], &last, sizeof(last));
        int end2 = memcmp(&expected_elements[i], &last, sizeof(last));
        int status;

        if (!end1 && !end2) break;

        status = !end1 ^ !end2;
        ok(!status, "Line %u, test %u: Mismatch in size, test declaration is %s than expected.\n",
                line, test_id, end1 ? "shorter" : "longer");
        if (status)
        {
            print_elements(elements);
            break;
        }

        status = memcmp(&elements[i], &expected_elements[i], sizeof(D3DVERTEXELEMENT9));
        ok(!status, "Line %u, test %u: Mismatch in element %u.\n", line, test_id, i);
        if (status)
        {
            print_elements(elements);
            break;
        }
    }
}

static void test_fvf_to_decl(DWORD test_fvf, const D3DVERTEXELEMENT9 expected_elements[],
        HRESULT expected_hr, unsigned int line, unsigned int test_id)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];

    hr = D3DXDeclaratorFromFVF(test_fvf, decl);
    ok(hr == expected_hr,
            "Line %u, test %u: D3DXDeclaratorFromFVF returned %#lx, expected %#lx.\n",
            line, test_id, hr, expected_hr);
    if (SUCCEEDED(hr)) compare_elements(decl, expected_elements, line, test_id);
}

static void test_decl_to_fvf(const D3DVERTEXELEMENT9 *decl, DWORD expected_fvf,
        HRESULT expected_hr, unsigned int line, unsigned int test_id)
{
    HRESULT hr;
    DWORD result_fvf = 0xdeadbeef;

    hr = D3DXFVFFromDeclarator(decl, &result_fvf);
    ok(hr == expected_hr,
       "Line %u, test %u: D3DXFVFFromDeclarator returned %#lx, expected %#lx.\n",
       line, test_id, hr, expected_hr);
    if (SUCCEEDED(hr))
    {
        ok(expected_fvf == result_fvf, "Line %u, test %u: Got FVF %#lx, expected %#lx.\n",
                line, test_id, result_fvf, expected_fvf);
    }
}

static void test_fvf_decl_conversion(void)
{
    static const struct
    {
        D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH + 1];
        DWORD fvf;
    }
    test_data[] =
    {
        {{
            D3DDECL_END(),
        }, 0},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZ},
        {{
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZRHW},
        {{
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZRHW},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB1 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB1 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB2},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 16, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB2 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB2 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB3},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 20, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB3 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB3 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 24, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB4 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 24, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB4 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB5 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB5 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            D3DDECL_END(),
        }, D3DFVF_NORMAL},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END(),
        }, D3DFVF_NORMAL | D3DFVF_DIFFUSE},
        {{
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0},
            D3DDECL_END(),
        }, D3DFVF_PSIZE},
        {{
            {0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END(),
        }, D3DFVF_DIFFUSE},
        {{
            {0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            D3DDECL_END(),
        }, D3DFVF_SPECULAR},
        /* Make sure textures of different sizes work. */
        {{
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEX1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEX1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEX1},
        /* Make sure the TEXCOORD index works correctly - try several textures. */
        {{
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 1},
            {0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 2},
            {0, 24, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3},
            D3DDECL_END(),
        }, D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEXCOORDSIZE3(1)
                | D3DFVF_TEXCOORDSIZE2(2) | D3DFVF_TEXCOORDSIZE4(3)},
        /* Now try some combination tests. */
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 32, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            {0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 44, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 1},
            D3DDECL_END(),
        }, D3DFVF_XYZB4 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2
                | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1)},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 24, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            {0, 32, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 36, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1},
            D3DDECL_END(),
        }, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_PSIZE | D3DFVF_SPECULAR | D3DFVF_TEX2
                | D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEXCOORDSIZE4(1)},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        test_decl_to_fvf(test_data[i].decl, test_data[i].fvf, D3D_OK, __LINE__, i);
        test_fvf_to_decl(test_data[i].fvf, test_data[i].decl, D3D_OK, __LINE__, i);
    }

    /* Usage indices for position and normal are apparently ignored. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 1},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZ, D3D_OK, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 1},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_NORMAL, D3D_OK, __LINE__, 0);
    }
    /* D3DFVF_LASTBETA_UBYTE4 and D3DFVF_LASTBETA_D3DCOLOR are ignored if
     * there are no blend matrices. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_fvf_to_decl(D3DFVF_XYZ | D3DFVF_LASTBETA_UBYTE4, decl, D3D_OK, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_fvf_to_decl(D3DFVF_XYZ | D3DFVF_LASTBETA_D3DCOLOR, decl, D3D_OK, __LINE__, 0);
    }
    /* D3DFVF_LASTBETA_UBYTE4 takes precedence over D3DFVF_LASTBETA_D3DCOLOR. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        };
        test_fvf_to_decl(D3DFVF_XYZB5 | D3DFVF_LASTBETA_D3DCOLOR | D3DFVF_LASTBETA_UBYTE4,
                decl, D3D_OK, __LINE__, 0);
    }
    /* These are supposed to fail, both ways. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZW, D3DERR_INVALIDCALL, __LINE__, 0);
        test_fvf_to_decl(D3DFVF_XYZW, decl, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 16, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZW | D3DFVF_NORMAL, D3DERR_INVALIDCALL, __LINE__, 0);
        test_fvf_to_decl(D3DFVF_XYZW | D3DFVF_NORMAL, decl, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZB5, D3DERR_INVALIDCALL, __LINE__, 0);
        test_fvf_to_decl(D3DFVF_XYZB5, decl, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Test a declaration that can't be converted to an FVF. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 24, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            {0, 32, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            /* 8 bytes padding */
            {0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Elements must be ordered by offset. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Basic tests for element order. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 16, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Textures must be ordered by texcoords. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 2},
            {0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 1},
            {0, 24, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Duplicate elements are not allowed. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Invalid FVFs cannot be converted to a declarator. */
    test_fvf_to_decl(0xdeadbeef, NULL, D3DERR_INVALIDCALL, __LINE__, 0);
}

static void D3DXGetFVFVertexSizeTest(void)
{
    UINT got;

    compare_vertex_sizes (D3DFVF_XYZ, 12);

    compare_vertex_sizes (D3DFVF_XYZB3, 24);

    compare_vertex_sizes (D3DFVF_XYZB5, 32);

    compare_vertex_sizes (D3DFVF_XYZ | D3DFVF_NORMAL, 24);

    compare_vertex_sizes (D3DFVF_XYZ | D3DFVF_DIFFUSE, 16);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE1(0), 16);
    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX2 |
        D3DFVF_TEXCOORDSIZE1(0) |
        D3DFVF_TEXCOORDSIZE1(1), 20);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE2(0), 20);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX2 |
        D3DFVF_TEXCOORDSIZE2(0) |
        D3DFVF_TEXCOORDSIZE2(1), 28);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX6 |
        D3DFVF_TEXCOORDSIZE2(0) |
        D3DFVF_TEXCOORDSIZE2(1) |
        D3DFVF_TEXCOORDSIZE2(2) |
        D3DFVF_TEXCOORDSIZE2(3) |
        D3DFVF_TEXCOORDSIZE2(4) |
        D3DFVF_TEXCOORDSIZE2(5), 60);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX8 |
        D3DFVF_TEXCOORDSIZE2(0) |
        D3DFVF_TEXCOORDSIZE2(1) |
        D3DFVF_TEXCOORDSIZE2(2) |
        D3DFVF_TEXCOORDSIZE2(3) |
        D3DFVF_TEXCOORDSIZE2(4) |
        D3DFVF_TEXCOORDSIZE2(5) |
        D3DFVF_TEXCOORDSIZE2(6) |
        D3DFVF_TEXCOORDSIZE2(7), 76);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE3(0), 24);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX4 |
        D3DFVF_TEXCOORDSIZE3(0) |
        D3DFVF_TEXCOORDSIZE3(1) |
        D3DFVF_TEXCOORDSIZE3(2) |
        D3DFVF_TEXCOORDSIZE3(3), 60);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE4(0), 28);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX2 |
        D3DFVF_TEXCOORDSIZE4(0) |
        D3DFVF_TEXCOORDSIZE4(1), 44);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX3 |
        D3DFVF_TEXCOORDSIZE4(0) |
        D3DFVF_TEXCOORDSIZE4(1) |
        D3DFVF_TEXCOORDSIZE4(2), 60);

    compare_vertex_sizes (
        D3DFVF_XYZB5 |
        D3DFVF_NORMAL |
        D3DFVF_DIFFUSE |
        D3DFVF_SPECULAR |
        D3DFVF_TEX8 |
        D3DFVF_TEXCOORDSIZE4(0) |
        D3DFVF_TEXCOORDSIZE4(1) |
        D3DFVF_TEXCOORDSIZE4(2) |
        D3DFVF_TEXCOORDSIZE4(3) |
        D3DFVF_TEXCOORDSIZE4(4) |
        D3DFVF_TEXCOORDSIZE4(5) |
        D3DFVF_TEXCOORDSIZE4(6) |
        D3DFVF_TEXCOORDSIZE4(7), 180);
}

static void D3DXIntersectTriTest(void)
{
    BOOL exp_res, got_res;
    D3DXVECTOR3 position, ray, vertex[3];
    FLOAT exp_dist, got_dist, exp_u, got_u, exp_v, got_v;

    vertex[0].x = 1.0f; vertex[0].y = 0.0f; vertex[0].z = 0.0f;
    vertex[1].x = 2.0f; vertex[1].y = 0.0f; vertex[1].z = 0.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 0.0f;

    position.x = -14.5f; position.y = -23.75f; position.z = -32.0f;

    ray.x = 2.0f; ray.y = 3.0f; ray.z = 4.0f;

    exp_res = TRUE; exp_u = 0.5f; exp_v = 0.25f; exp_dist = 8.0f;

    got_res = D3DXIntersectTri(&vertex[0], &vertex[1], &vertex[2], &position, &ray, &got_u, &got_v, &got_dist);
    ok(got_res == exp_res, "Expected result %d, got %d.\n", exp_res, got_res);
    ok(compare(exp_u, got_u), "Expected u %f, got %f.\n", exp_u, got_u);
    ok(compare(exp_v, got_v), "Expected v %f, got %f.\n", exp_v, got_v);
    ok(compare(exp_dist, got_dist), "Expected distance %f, got %f.\n", exp_dist, got_dist);

    got_res = D3DXIntersectTri(&vertex[0], &vertex[1], &vertex[2], &position, &ray, NULL, NULL, NULL);
    ok(got_res == exp_res, "Expected result %d, got %d.\n", exp_res, got_res);

    vertex[2].x = 1.0f; vertex[2].y = 0.0f; vertex[2].z = 0.0f;
    vertex[1].x = 2.0f; vertex[1].y = 0.0f; vertex[1].z = 0.0f;
    vertex[0].x = 1.0f; vertex[0].y = 1.0f; vertex[0].z = 0.0f;

    got_u = got_v = got_dist = 0.0f;
    got_res = D3DXIntersectTri(&vertex[0], &vertex[1], &vertex[2], &position, &ray, &got_u, &got_v, &got_dist);
    ok(got_res == exp_res, "Expected result %d, got %d.\n", exp_res, got_res);
    ok(compare(exp_u, got_u), "Expected u %f, got %f.\n", exp_u, got_u);
    ok(compare(exp_v, got_v), "Expected v %f, got %f.\n", exp_v, got_v);
    ok(compare(exp_dist, got_dist), "Expected distance %f, got %f.\n", exp_dist, got_dist);

    vertex[2].x = 1.0f; vertex[2].y = 0.0f; vertex[2].z = 0.0f;
    vertex[1].x = 2.0f; vertex[1].y = 0.0f; vertex[1].z = -0.5f;
    vertex[0].x = 1.0f; vertex[0].y = 1.0f; vertex[0].z = -1.0f;
    exp_u = 0.375f;
    exp_v = 0.5625f;
    exp_dist = 7.9375f;
    got_u = got_v = got_dist = 0.0f;
    got_res = D3DXIntersectTri(&vertex[0], &vertex[1], &vertex[2], &position, &ray, &got_u, &got_v, &got_dist);
    ok(got_res == exp_res, "Expected result %d, got %d.\n", exp_res, got_res);
    ok(compare(exp_u, got_u), "Expected u %f, got %f.\n", exp_u, got_u);
    ok(compare(exp_v, got_v), "Expected v %f, got %f.\n", exp_v, got_v);
    ok(compare(exp_dist, got_dist), "Expected distance %f, got %f.\n", exp_dist, got_dist);


/*Only positive ray is taken in account*/

    vertex[0].x = 1.0f; vertex[0].y = 0.0f; vertex[0].z = 0.0f;
    vertex[1].x = 2.0f; vertex[1].y = 0.0f; vertex[1].z = 0.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 0.0f;

    position.x = 17.5f; position.y = 24.25f; position.z = 32.0f;

    ray.x = 2.0f; ray.y = 3.0f; ray.z = 4.0f;

    exp_res = FALSE;

    got_res = D3DXIntersectTri(&vertex[0],&vertex[1],&vertex[2],&position,&ray,&got_u,&got_v,&got_dist);
    ok( got_res == exp_res, "Expected result = %d, got %d\n",exp_res,got_res);

    got_res = D3DXIntersectTri(&vertex[0], &vertex[1], &vertex[2], &position, &ray, NULL, NULL, NULL);
    ok(got_res == exp_res, "Expected result = %d, got %d\n", exp_res, got_res);

/*Intersection between ray and triangle in a same plane is considered as empty*/

    vertex[0].x = 4.0f; vertex[0].y = 0.0f; vertex[0].z = 0.0f;
    vertex[1].x = 6.0f; vertex[1].y = 0.0f; vertex[1].z = 0.0f;
    vertex[2].x = 4.0f; vertex[2].y = 2.0f; vertex[2].z = 0.0f;

    position.x = 1.0f; position.y = 1.0f; position.z = 0.0f;

    ray.x = 1.0f; ray.y = 0.0f; ray.z = 0.0f;

    exp_res = FALSE;

    got_res = D3DXIntersectTri(&vertex[0],&vertex[1],&vertex[2],&position,&ray,&got_u,&got_v,&got_dist);
    ok( got_res == exp_res, "Expected result = %d, got %d\n",exp_res,got_res);

    got_res = D3DXIntersectTri(&vertex[0], &vertex[1], &vertex[2], &position, &ray, NULL, NULL, NULL);
    ok(got_res == exp_res, "Expected result = %d, got %d\n", exp_res, got_res);
}

static void D3DXCreateMeshTest(void)
{
    HRESULT hr;
    IDirect3DDevice9 *device, *test_device;
    ID3DXMesh *d3dxmesh;
    int i, size;
    D3DVERTEXELEMENT9 test_decl[MAX_FVF_DECL_SIZE];
    DWORD options;
    struct mesh mesh;
    struct test_context *test_context;

    static const D3DVERTEXELEMENT9 decl1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(),
    };

    static const D3DVERTEXELEMENT9 decl2[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0},
        {0, 28, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
        {0, 32, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        /* 8 bytes padding */
        {0, 44, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END(),
    };

    static const D3DVERTEXELEMENT9 decl3[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(),
    };

    hr = D3DXCreateMesh(0, 0, 0, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl1, NULL, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateMesh(0, 3, D3DXMESH_MANAGED, decl1, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 0, D3DXMESH_MANAGED, decl1, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, 0, decl1, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);

    if (hr == D3D_OK)
    {
        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, 0, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl1, device, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl1, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

    if (hr == D3D_OK)
    {
        /* device */
        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, &test_device);
        ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
        ok(test_device == device, "Got result %p, expected %p\n", test_device, device);

        if (hr == D3D_OK)
        {
            IDirect3DDevice9_Release(device);
        }

        /* declaration */
        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, test_decl);
        ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

        if (hr == D3D_OK)
        {
            size = ARRAY_SIZE(decl1);
            for (i = 0; i < size - 1; i++)
            {
                ok(test_decl[i].Stream == decl1[i].Stream, "Returned stream %d, expected %d\n", test_decl[i].Stream, decl1[i].Stream);
                ok(test_decl[i].Type == decl1[i].Type, "Returned type %d, expected %d\n", test_decl[i].Type, decl1[i].Type);
                ok(test_decl[i].Method == decl1[i].Method, "Returned method %d, expected %d\n", test_decl[i].Method, decl1[i].Method);
                ok(test_decl[i].Usage == decl1[i].Usage, "Returned usage %d, expected %d\n", test_decl[i].Usage, decl1[i].Usage);
                ok(test_decl[i].UsageIndex == decl1[i].UsageIndex, "Returned usage index %d, expected %d\n", test_decl[i].UsageIndex, decl1[i].UsageIndex);
                ok(test_decl[i].Offset == decl1[i].Offset, "Returned offset %d, expected %d\n", test_decl[i].Offset, decl1[i].Offset);
            }
            ok(decl1[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }

        /* options */
        options = d3dxmesh->lpVtbl->GetOptions(d3dxmesh);
        ok(options == D3DXMESH_MANAGED, "Got result %lx, expected %x (D3DXMESH_MANAGED)\n", options, D3DXMESH_MANAGED);

        /* rest */
        if (!new_mesh(&mesh, 3, 1))
        {
            skip("Couldn't create mesh\n");
        }
        else
        {
            memset(mesh.vertices, 0, mesh.number_of_vertices * sizeof(*mesh.vertices));
            memset(mesh.faces, 0, mesh.number_of_faces * sizeof(*mesh.faces));
            mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

            compare_mesh("createmesh1", d3dxmesh, &mesh);

            free_mesh(&mesh);
        }

        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    /* Test a declaration that can't be converted to an FVF. */
    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl2, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

    if (hr == D3D_OK)
    {
        /* device */
        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, &test_device);
        ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
        ok(test_device == device, "Got result %p, expected %p\n", test_device, device);

        if (hr == D3D_OK)
        {
            IDirect3DDevice9_Release(device);
        }

        /* declaration */
        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, test_decl);
        ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

        if (hr == D3D_OK)
        {
            size = ARRAY_SIZE(decl2);
            for (i = 0; i < size - 1; i++)
            {
                ok(test_decl[i].Stream == decl2[i].Stream, "Returned stream %d, expected %d\n", test_decl[i].Stream, decl2[i].Stream);
                ok(test_decl[i].Type == decl2[i].Type, "Returned type %d, expected %d\n", test_decl[i].Type, decl2[i].Type);
                ok(test_decl[i].Method == decl2[i].Method, "Returned method %d, expected %d\n", test_decl[i].Method, decl2[i].Method);
                ok(test_decl[i].Usage == decl2[i].Usage, "Returned usage %d, expected %d\n", test_decl[i].Usage, decl2[i].Usage);
                ok(test_decl[i].UsageIndex == decl2[i].UsageIndex, "Returned usage index %d, expected %d\n", test_decl[i].UsageIndex, decl2[i].UsageIndex);
                ok(test_decl[i].Offset == decl2[i].Offset, "Returned offset %d, expected %d\n", test_decl[i].Offset, decl2[i].Offset);
            }
            ok(decl2[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }

        /* options */
        options = d3dxmesh->lpVtbl->GetOptions(d3dxmesh);
        ok(options == D3DXMESH_MANAGED, "Got result %lx, expected %x (D3DXMESH_MANAGED)\n", options, D3DXMESH_MANAGED);

        /* rest */
        if (!new_mesh(&mesh, 3, 1))
        {
            skip("Couldn't create mesh\n");
        }
        else
        {
            memset(mesh.vertices, 0, mesh.number_of_vertices * sizeof(*mesh.vertices));
            memset(mesh.faces, 0, mesh.number_of_faces * sizeof(*mesh.faces));
            mesh.fvf = 0;
            mesh.vertex_size = 60;

            compare_mesh("createmesh2", d3dxmesh, &mesh);

            free_mesh(&mesh);
        }

        mesh.vertex_size = d3dxmesh->lpVtbl->GetNumBytesPerVertex(d3dxmesh);
        ok(mesh.vertex_size == 60, "Got vertex size %u, expected %u\n", mesh.vertex_size, 60);

        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    /* Test a declaration with multiple streams. */
    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl3, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    free_test_context(test_context);
}

static void D3DXCreateMeshFVFTest(void)
{
    HRESULT hr;
    IDirect3DDevice9 *device, *test_device;
    ID3DXMesh *d3dxmesh;
    int i, size;
    D3DVERTEXELEMENT9 test_decl[MAX_FVF_DECL_SIZE];
    DWORD options;
    struct mesh mesh;
    struct test_context *test_context;

    static const D3DVERTEXELEMENT9 decl[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(),
    };

    hr = D3DXCreateMeshFVF(0, 0, 0, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, NULL, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateMeshFVF(0, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 0, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, 0, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);

    if (hr == D3D_OK)
    {
        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, 0xdeadbeef, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

    if (hr == D3D_OK)
    {
        /* device */
        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, &test_device);
        ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
        ok(test_device == device, "Got result %p, expected %p\n", test_device, device);

        if (hr == D3D_OK)
        {
            IDirect3DDevice9_Release(device);
        }

        /* declaration */
        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, test_decl);
        ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

        if (hr == D3D_OK)
        {
            size = ARRAY_SIZE(decl);
            for (i = 0; i < size - 1; i++)
            {
                ok(test_decl[i].Stream == decl[i].Stream, "Returned stream %d, expected %d\n", test_decl[i].Stream, decl[i].Stream);
                ok(test_decl[i].Type == decl[i].Type, "Returned type %d, expected %d\n", test_decl[i].Type, decl[i].Type);
                ok(test_decl[i].Method == decl[i].Method, "Returned method %d, expected %d\n", test_decl[i].Method, decl[i].Method);
                ok(test_decl[i].Usage == decl[i].Usage, "Returned usage %d, expected %d\n", test_decl[i].Usage, decl[i].Usage);
                ok(test_decl[i].UsageIndex == decl[i].UsageIndex, "Returned usage index %d, expected %d\n",
                   test_decl[i].UsageIndex, decl[i].UsageIndex);
                ok(test_decl[i].Offset == decl[i].Offset, "Returned offset %d, expected %d\n", test_decl[i].Offset, decl[i].Offset);
            }
            ok(decl[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }

        /* options */
        options = d3dxmesh->lpVtbl->GetOptions(d3dxmesh);
        ok(options == D3DXMESH_MANAGED, "Got result %lx, expected %x (D3DXMESH_MANAGED)\n", options, D3DXMESH_MANAGED);

        /* rest */
        if (!new_mesh(&mesh, 3, 1))
        {
            skip("Couldn't create mesh\n");
        }
        else
        {
            memset(mesh.vertices, 0, mesh.number_of_vertices * sizeof(*mesh.vertices));
            memset(mesh.faces, 0, mesh.number_of_faces * sizeof(*mesh.faces));
            mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

            compare_mesh("createmeshfvf", d3dxmesh, &mesh);

            free_mesh(&mesh);
        }

        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    free_test_context(test_context);
}

#define check_vertex_buffer(mesh, vertices, num_vertices, fvf) \
    check_vertex_buffer_(__LINE__, mesh, vertices, num_vertices, fvf)
static void check_vertex_buffer_(int line, ID3DXMesh *mesh, const void *vertices, DWORD num_vertices, DWORD fvf)
{
    DWORD mesh_num_vertices = mesh->lpVtbl->GetNumVertices(mesh);
    DWORD mesh_fvf = mesh->lpVtbl->GetFVF(mesh);
    const void *mesh_vertices;
    HRESULT hr;

    ok_(__FILE__,line)(fvf == mesh_fvf, "expected FVF %lx, got %lx\n", fvf, mesh_fvf);
    ok_(__FILE__,line)(num_vertices == mesh_num_vertices,
       "Expected %lu vertices, got %lu\n", num_vertices, mesh_num_vertices);

    hr = mesh->lpVtbl->LockVertexBuffer(mesh, D3DLOCK_READONLY, (void**)&mesh_vertices);
    ok_(__FILE__,line)(hr == D3D_OK, "LockVertexBuffer returned %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    if (FAILED(hr))
        return;

    if (mesh_fvf == fvf) {
        DWORD vertex_size = D3DXGetFVFVertexSize(fvf), i;

        for (i = 0; i < min(num_vertices, mesh_num_vertices); i++)
        {
            const FLOAT *exp_float = vertices;
            const FLOAT *got_float = mesh_vertices;
            DWORD texcount;
            DWORD pos_dim = 0;
            int j;
            BOOL last_beta_dword = FALSE;
            char prefix[128];

            switch (fvf & D3DFVF_POSITION_MASK) {
                case D3DFVF_XYZ: pos_dim = 3; break;
                case D3DFVF_XYZRHW: pos_dim = 4; break;
                case D3DFVF_XYZB1:
                case D3DFVF_XYZB2:
                case D3DFVF_XYZB3:
                case D3DFVF_XYZB4:
                case D3DFVF_XYZB5:
                    pos_dim = (fvf & D3DFVF_POSITION_MASK) - D3DFVF_XYZB1 + 1;
                    if (fvf & (D3DFVF_LASTBETA_UBYTE4 | D3DFVF_LASTBETA_D3DCOLOR))
                    {
                        pos_dim--;
                        last_beta_dword = TRUE;
                    }
                    break;
                case D3DFVF_XYZW: pos_dim = 4; break;
            }
            sprintf(prefix, "vertex[%lu] position, ", i);
            check_floats_(line, prefix, got_float, exp_float, pos_dim);
            exp_float += pos_dim;
            got_float += pos_dim;

            if (last_beta_dword) {
                ok_(__FILE__,line)(*(DWORD*)exp_float == *(DWORD*)got_float,
                    "Vertex[%lu]: Expected last beta %08lx, got %08lx\n", i, *(DWORD*)exp_float, *(DWORD*)got_float);
                exp_float++;
                got_float++;
            }

            if (fvf & D3DFVF_NORMAL) {
                sprintf(prefix, "vertex[%lu] normal, ", i);
                check_floats_(line, prefix, got_float, exp_float, 3);
                exp_float += 3;
                got_float += 3;
            }
            if (fvf & D3DFVF_PSIZE) {
                ok_(__FILE__,line)(compare(*exp_float, *got_float),
                        "Vertex[%lu]: Expected psize %g, got %g\n", i, *exp_float, *got_float);
                exp_float++;
                got_float++;
            }
            if (fvf & D3DFVF_DIFFUSE) {
                ok_(__FILE__,line)(*(DWORD*)exp_float == *(DWORD*)got_float,
                    "Vertex[%lu]: Expected diffuse %08lx, got %08lx\n", i, *(DWORD*)exp_float, *(DWORD*)got_float);
                exp_float++;
                got_float++;
            }
            if (fvf & D3DFVF_SPECULAR) {
                ok_(__FILE__,line)(*(DWORD*)exp_float == *(DWORD*)got_float,
                    "Vertex[%lu]: Expected specular %08lx, got %08lx\n", i, *(DWORD*)exp_float, *(DWORD*)got_float);
                exp_float++;
                got_float++;
            }

            texcount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
            for (j = 0; j < texcount; j++) {
                DWORD dim = (((fvf >> (16 + 2 * j)) + 1) & 0x03) + 1;
                sprintf(prefix, "vertex[%lu] texture, ", i);
                check_floats_(line, prefix, got_float, exp_float, dim);
                exp_float += dim;
                got_float += dim;
            }

            vertices = (BYTE*)vertices + vertex_size;
            mesh_vertices = (BYTE*)mesh_vertices + vertex_size;
        }
    }

    mesh->lpVtbl->UnlockVertexBuffer(mesh);
}

#define check_index_buffer(mesh, indices, num_indices, index_size) \
    check_index_buffer_(__LINE__, mesh, indices, num_indices, index_size)
static void check_index_buffer_(int line, ID3DXMesh *mesh, const void *indices, DWORD num_indices, DWORD index_size)
{
    DWORD mesh_index_size = (mesh->lpVtbl->GetOptions(mesh) & D3DXMESH_32BIT) ? 4 : 2;
    DWORD mesh_num_indices = mesh->lpVtbl->GetNumFaces(mesh) * 3;
    const void *mesh_indices;
    HRESULT hr;
    DWORD i;

    ok_(__FILE__,line)(index_size == mesh_index_size,
        "Expected index size %lu, got %lu\n", index_size, mesh_index_size);
    ok_(__FILE__,line)(num_indices == mesh_num_indices,
        "Expected %lu indices, got %lu\n", num_indices, mesh_num_indices);

    hr = mesh->lpVtbl->LockIndexBuffer(mesh, D3DLOCK_READONLY, (void**)&mesh_indices);
    ok_(__FILE__,line)(hr == D3D_OK, "LockIndexBuffer returned %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    if (FAILED(hr))
        return;

    if (mesh_index_size == index_size) {
        for (i = 0; i < min(num_indices, mesh_num_indices); i++)
        {
            if (index_size == 4)
                ok_(__FILE__,line)(*(DWORD*)indices == *(DWORD*)mesh_indices,
                    "Index[%lu]: expected %lu, got %lu\n", i, *(DWORD*)indices, *(DWORD*)mesh_indices);
            else
                ok_(__FILE__,line)(*(WORD*)indices == *(WORD*)mesh_indices,
                    "Index[%lu]: expected %u, got %u\n", i, *(WORD*)indices, *(WORD*)mesh_indices);
            indices = (BYTE*)indices + index_size;
            mesh_indices = (BYTE*)mesh_indices + index_size;
        }
    }
    mesh->lpVtbl->UnlockIndexBuffer(mesh);
}

#define check_matrix(got, expected) check_matrix_(__LINE__, got, expected)
static void check_matrix_(int line, const D3DXMATRIX *got, const D3DXMATRIX *expected)
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            ok_(__FILE__,line)(compare(expected->m[i][j], got->m[i][j]),
                    "matrix[%u][%u]: expected %g, got %g\n",
                    i, j, expected->m[i][j], got->m[i][j]);
        }
    }
}

static void check_colorvalue_(int line, const char *prefix, const D3DCOLORVALUE got, const D3DCOLORVALUE expected)
{
    ok_(__FILE__,line)(expected.r == got.r && expected.g == got.g && expected.b == got.b && expected.a == got.a,
            "%sExpected (%g, %g, %g, %g), got (%g, %g, %g, %g)\n", prefix,
            expected.r, expected.g, expected.b, expected.a, got.r, got.g, got.b, got.a);
}

#define check_materials(got, got_count, expected, expected_count) \
    check_materials_(__LINE__, got, got_count, expected, expected_count)
static void check_materials_(int line, const D3DXMATERIAL *got, DWORD got_count, const D3DXMATERIAL *expected, DWORD expected_count)
{
    int i;
    ok_(__FILE__,line)(expected_count == got_count, "Expected %lu materials, got %lu\n", expected_count, got_count);
    if (!expected) {
        ok_(__FILE__,line)(got == NULL, "Expected NULL material ptr, got %p\n", got);
        return;
    }
    for (i = 0; i < min(expected_count, got_count); i++)
    {
        if (!expected[i].pTextureFilename)
            ok_(__FILE__,line)(got[i].pTextureFilename == NULL,
                    "Expected NULL pTextureFilename, got %p\n", got[i].pTextureFilename);
        else
            ok_(__FILE__,line)(!strcmp(expected[i].pTextureFilename, got[i].pTextureFilename),
                    "Expected '%s' for pTextureFilename, got '%s'\n", expected[i].pTextureFilename, got[i].pTextureFilename);
        check_colorvalue_(line, "Diffuse: ", got[i].MatD3D.Diffuse, expected[i].MatD3D.Diffuse);
        check_colorvalue_(line, "Ambient: ", got[i].MatD3D.Ambient, expected[i].MatD3D.Ambient);
        check_colorvalue_(line, "Specular: ", got[i].MatD3D.Specular, expected[i].MatD3D.Specular);
        check_colorvalue_(line, "Emissive: ", got[i].MatD3D.Emissive, expected[i].MatD3D.Emissive);
        ok_(__FILE__,line)(expected[i].MatD3D.Power == got[i].MatD3D.Power,
                "Power: Expected %g, got %g\n", expected[i].MatD3D.Power, got[i].MatD3D.Power);
    }
}

#define check_generated_adjacency(mesh, got, epsilon) check_generated_adjacency_(__LINE__, mesh, got, epsilon)
static void check_generated_adjacency_(int line, ID3DXMesh *mesh, const DWORD *got, FLOAT epsilon)
{
    DWORD *expected;
    DWORD num_faces = mesh->lpVtbl->GetNumFaces(mesh);
    HRESULT hr;

    expected = malloc(num_faces * sizeof(DWORD) * 3);
    if (!expected) {
        skip_(__FILE__, line)("Out of memory\n");
        return;
    }
    hr = mesh->lpVtbl->GenerateAdjacency(mesh, epsilon, expected);
    ok_(__FILE__, line)(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        int i;
        for (i = 0; i < num_faces; i++)
        {
            ok_(__FILE__, line)(expected[i * 3] == got[i * 3] &&
                    expected[i * 3 + 1] == got[i * 3 + 1] &&
                    expected[i * 3 + 2] == got[i * 3 + 2],
                    "Face %u adjacencies: Expected (%lu, %lu, %lu), got (%lu, %lu, %lu)\n", i,
                    expected[i * 3], expected[i * 3 + 1], expected[i * 3 + 2],
                    got[i * 3], got[i * 3 + 1], got[i * 3 + 2]);
        }
    }
    free(expected);
}

#define check_generated_effects(materials, num_materials, effects) \
    check_generated_effects_(__LINE__, materials, num_materials, effects)
static void check_generated_effects_(int line, const D3DXMATERIAL *materials, DWORD num_materials, const D3DXEFFECTINSTANCE *effects)
{
    int i;
    static const struct {
        const char *name;
        DWORD name_size;
        DWORD num_bytes;
        DWORD value_offset;
    } params[] = {
#define EFFECT_TABLE_ENTRY(str, field) \
    {str, sizeof(str), sizeof(materials->MatD3D.field), offsetof(D3DXMATERIAL, MatD3D.field)}
        EFFECT_TABLE_ENTRY("Diffuse", Diffuse),
        EFFECT_TABLE_ENTRY("Power", Power),
        EFFECT_TABLE_ENTRY("Specular", Specular),
        EFFECT_TABLE_ENTRY("Emissive", Emissive),
        EFFECT_TABLE_ENTRY("Ambient", Ambient),
#undef EFFECT_TABLE_ENTRY
    };

    if (!num_materials) {
        ok_(__FILE__, line)(effects == NULL, "Expected NULL effects, got %p\n", effects);
        return;
    }
    for (i = 0; i < num_materials; i++)
    {
        int j;
        DWORD expected_num_defaults = ARRAY_SIZE(params) + (materials[i].pTextureFilename ? 1 : 0);

        ok_(__FILE__,line)(expected_num_defaults == effects[i].NumDefaults,
                "effect[%u] NumDefaults: Expected %lu, got %lu\n", i,
                expected_num_defaults, effects[i].NumDefaults);
        for (j = 0; j < min(ARRAY_SIZE(params), effects[i].NumDefaults); j++)
        {
            int k;
            D3DXEFFECTDEFAULT *got_param = &effects[i].pDefaults[j];
            ok_(__FILE__,line)(!strcmp(params[j].name, got_param->pParamName),
               "effect[%u].pDefaults[%u].pParamName: Expected '%s', got '%s'\n", i, j,
               params[j].name, got_param->pParamName);
            ok_(__FILE__,line)(D3DXEDT_FLOATS == got_param->Type,
               "effect[%u].pDefaults[%u].Type: Expected %u, got %u\n", i, j,
               D3DXEDT_FLOATS, got_param->Type);
            ok_(__FILE__,line)(params[j].num_bytes == got_param->NumBytes,
               "effect[%u].pDefaults[%u].NumBytes: Expected %lu, got %lu\n", i, j,
               params[j].num_bytes, got_param->NumBytes);
            for (k = 0; k < min(params[j].num_bytes, got_param->NumBytes) / 4; k++)
            {
                FLOAT expected = ((FLOAT*)((BYTE*)&materials[i] + params[j].value_offset))[k];
                FLOAT got = ((FLOAT*)got_param->pValue)[k];
                ok_(__FILE__,line)(compare(expected, got),
                   "effect[%u].pDefaults[%u] float value %u: Expected %g, got %g\n", i, j, k, expected, got);
            }
        }
        if (effects[i].NumDefaults > ARRAY_SIZE(params)) {
            D3DXEFFECTDEFAULT *got_param = &effects[i].pDefaults[j];
            static const char *expected_name = "Texture0@Name";

            ok_(__FILE__,line)(!strcmp(expected_name, got_param->pParamName),
               "effect[%u].pDefaults[%u].pParamName: Expected '%s', got '%s'\n", i, j,
               expected_name, got_param->pParamName);
            ok_(__FILE__,line)(D3DXEDT_STRING == got_param->Type,
               "effect[%u].pDefaults[%u].Type: Expected %u, got %u\n", i, j,
               D3DXEDT_STRING, got_param->Type);
            if (materials[i].pTextureFilename) {
                ok_(__FILE__,line)(strlen(materials[i].pTextureFilename) + 1 == got_param->NumBytes,
                   "effect[%u] texture filename length: Expected %lu, got %lu\n", i,
                   (DWORD)strlen(materials[i].pTextureFilename) + 1, got_param->NumBytes);
                ok_(__FILE__,line)(!strcmp(materials[i].pTextureFilename, got_param->pValue),
                   "effect[%u] texture filename: Expected '%s', got '%s'\n", i,
                   materials[i].pTextureFilename, (char*)got_param->pValue);
            }
        }
    }
}

static HRESULT CALLBACK ID3DXAllocateHierarchyImpl_DestroyFrame(ID3DXAllocateHierarchy *iface, LPD3DXFRAME frame)
{
    TRACECALLBACK("ID3DXAllocateHierarchyImpl_DestroyFrame(%p, %p)\n", iface, frame);
    if (frame) {
        free(frame->Name);
        free(frame);
    }
    return D3D_OK;
}

static HRESULT CALLBACK ID3DXAllocateHierarchyImpl_CreateFrame(ID3DXAllocateHierarchy *iface,
        const char *name, D3DXFRAME **new_frame)
{
    D3DXFRAME *frame;

    TRACECALLBACK("ID3DXAllocateHierarchyImpl_CreateFrame(%p, '%s', %p)\n", iface, name, new_frame);
    frame = calloc(1, sizeof(*frame));
    if (!frame)
        return E_OUTOFMEMORY;
    if (name) {
        frame->Name = strdup(name);
        if (!frame->Name) {
            free(frame);
            return E_OUTOFMEMORY;
        }
    }
    *new_frame = frame;
    return D3D_OK;
}

static HRESULT destroy_mesh_container(LPD3DXMESHCONTAINER mesh_container)
{
    int i;

    if (!mesh_container)
        return D3D_OK;
    free(mesh_container->Name);
    if (mesh_container->MeshData.pMesh)
        IUnknown_Release(mesh_container->MeshData.pMesh);
    if (mesh_container->pMaterials) {
        for (i = 0; i < mesh_container->NumMaterials; i++)
            free(mesh_container->pMaterials[i].pTextureFilename);
        free(mesh_container->pMaterials);
    }
    if (mesh_container->pEffects) {
        for (i = 0; i < mesh_container->NumMaterials; i++) {
            free(mesh_container->pEffects[i].pEffectFilename);
            if (mesh_container->pEffects[i].pDefaults) {
                int j;
                for (j = 0; j < mesh_container->pEffects[i].NumDefaults; j++) {
                    free(mesh_container->pEffects[i].pDefaults[j].pParamName);
                    free(mesh_container->pEffects[i].pDefaults[j].pValue);
                }
                free(mesh_container->pEffects[i].pDefaults);
            }
        }
        free(mesh_container->pEffects);
    }
    free(mesh_container->pAdjacency);
    if (mesh_container->pSkinInfo)
        IUnknown_Release(mesh_container->pSkinInfo);
    free(mesh_container);
    return D3D_OK;
}

static HRESULT CALLBACK ID3DXAllocateHierarchyImpl_DestroyMeshContainer(ID3DXAllocateHierarchy *iface, LPD3DXMESHCONTAINER mesh_container)
{
    TRACECALLBACK("ID3DXAllocateHierarchyImpl_DestroyMeshContainer(%p, %p)\n", iface, mesh_container);
    return destroy_mesh_container(mesh_container);
}

static HRESULT CALLBACK ID3DXAllocateHierarchyImpl_CreateMeshContainer(ID3DXAllocateHierarchy *iface,
        const char *name, const D3DXMESHDATA *mesh_data, const D3DXMATERIAL *materials,
        const D3DXEFFECTINSTANCE *effects, DWORD num_materials, const DWORD *adjacency,
        ID3DXSkinInfo *skin_info, D3DXMESHCONTAINER **new_mesh_container)
{
    LPD3DXMESHCONTAINER mesh_container = NULL;
    int i;

    TRACECALLBACK("ID3DXAllocateHierarchyImpl_CreateMeshContainer(%p, '%s', %u, %p, %p, %p, %ld, %p, %p, %p)\n",
            iface, name, mesh_data->Type, mesh_data->pMesh, materials, effects,
            num_materials, adjacency, skin_info, *new_mesh_container);

    mesh_container = calloc(1, sizeof(*mesh_container));
    if (!mesh_container)
        return E_OUTOFMEMORY;

    if (name) {
        mesh_container->Name = strdup(name);
        if (!mesh_container->Name)
            goto error;
    }

    mesh_container->NumMaterials = num_materials;
    if (num_materials) {
        mesh_container->pMaterials = malloc(num_materials * sizeof(*materials));
        if (!mesh_container->pMaterials)
            goto error;

        memcpy(mesh_container->pMaterials, materials, num_materials * sizeof(*materials));
        for (i = 0; i < num_materials; i++)
            mesh_container->pMaterials[i].pTextureFilename = NULL;
        for (i = 0; i < num_materials; i++) {
            if (materials[i].pTextureFilename) {
                mesh_container->pMaterials[i].pTextureFilename = strdup(materials[i].pTextureFilename);
                if (!mesh_container->pMaterials[i].pTextureFilename)
                    goto error;
            }
        }

        mesh_container->pEffects = calloc(num_materials, sizeof(*effects));
        if (!mesh_container->pEffects)
            goto error;
        for (i = 0; i < num_materials; i++) {
            int j;
            const D3DXEFFECTINSTANCE *effect_src = &effects[i];
            D3DXEFFECTINSTANCE *effect_dest = &mesh_container->pEffects[i];

            if (effect_src->pEffectFilename) {
                effect_dest->pEffectFilename = strdup(effect_src->pEffectFilename);
                if (!effect_dest->pEffectFilename)
                    goto error;
            }
            effect_dest->pDefaults = calloc(effect_src->NumDefaults, sizeof(*effect_src->pDefaults));
            if (!effect_dest->pDefaults)
                goto error;
            effect_dest->NumDefaults = effect_src->NumDefaults;
            for (j = 0; j < effect_src->NumDefaults; j++) {
                const D3DXEFFECTDEFAULT *default_src = &effect_src->pDefaults[j];
                D3DXEFFECTDEFAULT *default_dest = &effect_dest->pDefaults[j];

                if (default_src->pParamName) {
                    default_dest->pParamName = strdup(default_src->pParamName);
                    if (!default_dest->pParamName)
                        goto error;
                }
                default_dest->NumBytes = default_src->NumBytes;
                default_dest->Type = default_src->Type;
                default_dest->pValue = malloc(default_src->NumBytes);
                memcpy(default_dest->pValue, default_src->pValue, default_src->NumBytes);
            }
        }
    }

    ok(adjacency != NULL, "Expected non-NULL adjacency, got NULL\n");
    if (adjacency) {
        if (mesh_data->Type == D3DXMESHTYPE_MESH || mesh_data->Type == D3DXMESHTYPE_PMESH) {
            ID3DXBaseMesh *basemesh = (ID3DXBaseMesh*)mesh_data->pMesh;
            DWORD num_faces = basemesh->lpVtbl->GetNumFaces(basemesh);
            size_t size = num_faces * sizeof(DWORD) * 3;
            mesh_container->pAdjacency = malloc(size);
            if (!mesh_container->pAdjacency)
                goto error;
            memcpy(mesh_container->pAdjacency, adjacency, size);
        } else {
            ok(mesh_data->Type == D3DXMESHTYPE_PATCHMESH, "Unknown mesh type %u\n", mesh_data->Type);
            if (mesh_data->Type == D3DXMESHTYPE_PATCHMESH)
                trace("FIXME: copying adjacency data for patch mesh not implemented\n");
        }
    }

    memcpy(&mesh_container->MeshData, mesh_data, sizeof(*mesh_data));
    if (mesh_data->pMesh)
        IUnknown_AddRef(mesh_data->pMesh);
    if (skin_info) {
        mesh_container->pSkinInfo = skin_info;
        skin_info->lpVtbl->AddRef(skin_info);
    }
    *new_mesh_container = mesh_container;

    return S_OK;
error:
    destroy_mesh_container(mesh_container);
    return E_OUTOFMEMORY;
}

static ID3DXAllocateHierarchyVtbl ID3DXAllocateHierarchyImpl_Vtbl = {
    ID3DXAllocateHierarchyImpl_CreateFrame,
    ID3DXAllocateHierarchyImpl_CreateMeshContainer,
    ID3DXAllocateHierarchyImpl_DestroyFrame,
    ID3DXAllocateHierarchyImpl_DestroyMeshContainer,
};
static ID3DXAllocateHierarchy alloc_hier = { &ID3DXAllocateHierarchyImpl_Vtbl };

#define test_LoadMeshFromX(device, xfile_str, vertex_array, fvf, index_array, materials_array, check_adjacency) \
    test_LoadMeshFromX_(__LINE__, device, xfile_str, sizeof(xfile_str) - 1, vertex_array, ARRAY_SIZE(vertex_array), fvf, \
            index_array, ARRAY_SIZE(index_array), sizeof(*index_array), materials_array, ARRAY_SIZE(materials_array), \
            check_adjacency);
static void test_LoadMeshFromX_(int line, IDirect3DDevice9 *device, const char *xfile_str, size_t xfile_strlen,
        const void *vertices, DWORD num_vertices, DWORD fvf, const void *indices, DWORD num_indices, size_t index_size,
        const D3DXMATERIAL *expected_materials, DWORD expected_num_materials, BOOL check_adjacency)
{
    HRESULT hr;
    ID3DXBuffer *materials = NULL;
    ID3DXBuffer *effects = NULL;
    ID3DXBuffer *adjacency = NULL;
    ID3DXMesh *mesh = NULL;
    DWORD num_materials = 0;

    /* Adjacency is not checked when the X file contains multiple meshes,
     * since calling GenerateAdjacency on the merged mesh is not equivalent
     * to calling GenerateAdjacency on the individual meshes and then merging
     * the adjacency data. */
    hr = D3DXLoadMeshFromXInMemory(xfile_str, xfile_strlen, D3DXMESH_MANAGED, device,
            check_adjacency ? &adjacency : NULL, &materials, &effects, &num_materials, &mesh);
    ok_(__FILE__,line)(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    if (SUCCEEDED(hr)) {
        D3DXMATERIAL *materials_ptr = materials ? ID3DXBuffer_GetBufferPointer(materials) : NULL;
        D3DXEFFECTINSTANCE *effects_ptr = effects ? ID3DXBuffer_GetBufferPointer(effects) : NULL;
        DWORD *adjacency_ptr = check_adjacency ? ID3DXBuffer_GetBufferPointer(adjacency) : NULL;

        check_vertex_buffer_(line, mesh, vertices, num_vertices, fvf);
        check_index_buffer_(line, mesh, indices, num_indices, index_size);
        check_materials_(line, materials_ptr, num_materials, expected_materials, expected_num_materials);
        check_generated_effects_(line, materials_ptr, num_materials, effects_ptr);
        if (check_adjacency)
            check_generated_adjacency_(line, mesh, adjacency_ptr, 0.0f);

        if (materials) ID3DXBuffer_Release(materials);
        if (effects) ID3DXBuffer_Release(effects);
        if (adjacency) ID3DXBuffer_Release(adjacency);
        IUnknown_Release(mesh);
    }
}

#define MAX_USER_DATA_COUNT 32
enum user_data_type
{
    USER_DATA_TYPE_TOP,
    USER_DATA_TYPE_FRAME_CHILD,
    USER_DATA_TYPE_MESH_CHILD,
};

struct test_user_data
{
    enum user_data_type data_type;
    const GUID *type;
    SIZE_T size;
    unsigned int value;
    BOOL mesh_container;
    unsigned int num_materials;
};

struct test_load_user_data
{
    ID3DXLoadUserData iface;

    unsigned int data_count;
    struct test_user_data data[MAX_USER_DATA_COUNT];
    GUID guids[MAX_USER_DATA_COUNT];
};

static void record_common_user_data(struct test_load_user_data *data, ID3DXFileData *filedata,
        enum user_data_type data_type)
{
    struct test_user_data *d = &data->data[data->data_count];
    const void *ptr;
    HRESULT hr;
    SIZE_T sz;

    assert(data->data_count < MAX_USER_DATA_COUNT);

    d->data_type = data_type;
    hr = filedata->lpVtbl->GetType(filedata, &data->guids[data->data_count]);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = filedata->lpVtbl->Lock(filedata, &sz, &ptr);
    ok(hr == S_OK, "got %#lx.\n", hr);
    d->size = sz;
    ok(sz >= sizeof(int), "got %Iu.\n", sz);
    d->value = *(unsigned int *)ptr;
    hr = filedata->lpVtbl->Unlock(filedata);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ++data->data_count;
}

static struct test_load_user_data *impl_from_ID3DXLoadUserData(ID3DXLoadUserData *iface)
{
    return CONTAINING_RECORD(iface, struct test_load_user_data, iface);
}

static HRESULT STDMETHODCALLTYPE load_top_level_data(ID3DXLoadUserData *iface, ID3DXFileData *filedata)
{
    struct test_load_user_data *user_data = impl_from_ID3DXLoadUserData(iface);

    record_common_user_data(user_data, filedata, USER_DATA_TYPE_TOP);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE load_frame_child_data(ID3DXLoadUserData *iface, D3DXFRAME *frame,
        ID3DXFileData *filedata)
{
    struct test_load_user_data *user_data = impl_from_ID3DXLoadUserData(iface);

    ok(!frame->pFrameSibling, "got %p.\n", frame->pFrameSibling);
    ok(!frame->pFrameFirstChild, "got %p.\n", frame->pFrameFirstChild);

    user_data->data[user_data->data_count].mesh_container = !!frame->pMeshContainer;
    record_common_user_data(user_data, filedata, USER_DATA_TYPE_FRAME_CHILD);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE load_mesh_child_data(ID3DXLoadUserData *iface, D3DXMESHCONTAINER *mesh_container,
        ID3DXFileData *filedata)
{
    struct test_load_user_data *user_data = impl_from_ID3DXLoadUserData(iface);

    user_data->data[user_data->data_count].num_materials = mesh_container->NumMaterials;

    record_common_user_data(user_data, filedata, USER_DATA_TYPE_MESH_CHILD);
    return S_OK;
}

static const struct ID3DXLoadUserDataVtbl load_user_data_vtbl =
{
    load_top_level_data,
    load_frame_child_data,
    load_mesh_child_data,
};

static void init_load_user_data(struct test_load_user_data *data)
{
    memset(data, 0, sizeof(*data));
    data->iface.lpVtbl = &load_user_data_vtbl;
}

static void check_user_data(struct test_load_user_data *user_data, unsigned int expected_count,
        const struct test_user_data *expected)
{
    unsigned int i;

    ok(user_data->data_count == expected_count, "got %u, expected %u.\n", user_data->data_count, expected_count);
    for (i = 0; i < expected_count; ++i)
    {
        winetest_push_context("i %u", i);
        ok(user_data->data[i].data_type == expected[i].data_type, "got %u, expected %u.\n",
                user_data->data[i].data_type, expected[i].data_type);
        ok(IsEqualGUID(&user_data->guids[i], expected[i].type), "got %s, expected %s.\n",
                debugstr_guid(&user_data->guids[i]), debugstr_guid(expected[i].type));
        ok(user_data->data[i].size == expected[i].size, "got %Iu, expected %Iu.\n",
                user_data->data[i].size, expected[i].size);
        ok(user_data->data[i].value == expected[i].value, "got %u, expected %u.\n",
                user_data->data[i].value, expected[i].value);
        ok(user_data->data[i].mesh_container == expected[i].mesh_container, "got %u, expected %u.\n",
                user_data->data[i].mesh_container, expected[i].mesh_container);
        ok(user_data->data[i].num_materials == expected[i].num_materials, "got %u, expected %u.\n",
                user_data->data[i].num_materials, expected[i].num_materials);
        winetest_pop_context();
    }
}

DEFINE_GUID(TID_TestDataGuid, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11);

static void D3DXLoadMeshTest(void)
{
    static const char empty_xfile[] = "xof 0303txt 0032";
    /*________________________*/
    static const char simple_xfile[] =
        "xof 0303txt 0032"
        "Mesh {"
            "3;"
            "0.0; 0.0; 0.0;,"
            "0.0; 1.0; 0.0;,"
            "1.0; 1.0; 0.0;;"
            "1;"
            "3; 0, 1, 2;;"
        "}";
    static const WORD simple_index_buffer[] = {0, 1, 2};
    static const D3DXVECTOR3 simple_vertex_buffer[] = {
        {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}
    };
    const DWORD simple_fvf = D3DFVF_XYZ;
    static const char framed_xfile[] =
        "xof 0303txt 0032"
        "template TestData {"
            "<12345678-1234-1234-1234-111111111111>"
            "DWORD value;"
        "}"
        "TestData {"
            "1;;"
        "}"
        "Material {"
            /* ColorRGBA faceColor; */
            "0.0; 0.0; 1.0; 1.0;;"
            /* FLOAT power; */
            "0.5;"
            /* ColorRGB specularColor; */
            "1.0; 1.0; 1.0;;"
            /* ColorRGB emissiveColor; */
            "0.0; 0.0; 0.0;;"
        "}"

        "Frame {"
            "TestData {"
                "2;;"
            "}"
            "Mesh { 3; 0.0; 0.0; 0.0;, 0.0; 1.0; 0.0;, 1.0; 1.0; 0.0;; 1; 3; 0, 1, 2;;"
                "TestData {"
                    "3;;"
                "}"
            "}"
            "FrameTransformMatrix {" /* translation (0.0, 0.0, 2.0) */
              "1.0, 0.0, 0.0, 0.0,"
              "0.0, 1.0, 0.0, 0.0,"
              "0.0, 0.0, 1.0, 0.0,"
              "0.0, 0.0, 2.0, 1.0;;"
            "}"
            "TestData {"
                "4;;"
            "}"
            "Mesh { 3; 0.0; 0.0; 0.0;, 0.0; 1.0; 0.0;, 2.0; 1.0; 0.0;; 1; 3; 0, 1, 2;; }"
            "FrameTransformMatrix {" /* translation (0.0, 0.0, 3.0) */
              "1.0, 0.0, 0.0, 0.0,"
              "0.0, 1.0, 0.0, 0.0,"
              "0.0, 0.0, 1.0, 0.0,"
              "0.0, 0.0, 3.0, 1.0;;"
            "}"
            "Mesh { 3; 0.0; 0.0; 0.0;, 0.0; 1.0; 0.0;, 3.0; 1.0; 0.0;; 1; 3; 0, 1, 2;; }"
        "}";

    static const struct test_user_data framed_xfile_expected_user_data[] =
    {
        { USER_DATA_TYPE_TOP, &TID_TestDataGuid, 4, 1, 0, 0},
        { USER_DATA_TYPE_TOP, &TID_D3DRMMaterial, 44, 0, 0, 0},
        { USER_DATA_TYPE_FRAME_CHILD, &TID_TestDataGuid, 4, 2, 0, 0},
        { USER_DATA_TYPE_MESH_CHILD, &TID_TestDataGuid, 4, 3, 0, 0},
        { USER_DATA_TYPE_FRAME_CHILD, &TID_TestDataGuid, 4, 4, 1, 0},
    };

    static const char framed_xfile_empty[] =
            "xof 0303txt 0032"
            "Frame Box01 {"
            "    Mesh { 0;; 0;;"
            "        MeshNormals { 0;; 0;; }"
            "    }"
            "}";

    static const WORD framed_index_buffer[] = { 0, 1, 2 };
    static const D3DXVECTOR3 framed_vertex_buffers[3][3] = {
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}},
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {2.0, 1.0, 0.0}},
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {3.0, 1.0, 0.0}},
    };
    static const WORD merged_index_buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    /* frame transforms accumulates for D3DXLoadMeshFromX */
    static const D3DXVECTOR3 merged_vertex_buffer[] = {
        {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0},
        {0.0, 0.0, 2.0}, {0.0, 1.0, 2.0}, {2.0, 1.0, 2.0},
        {0.0, 0.0, 5.0}, {0.0, 1.0, 5.0}, {3.0, 1.0, 5.0},
    };
    const DWORD framed_fvf = D3DFVF_XYZ;
    /*________________________*/
    static const char box_xfile[] =
        "xof 0303txt 0032"
        "template TestData {"
            "<12345678-1234-1234-1234-111111111111>"
            "DWORD value;"
        "}"
        "Mesh {"
            "8;" /* DWORD nVertices; */
            /* array Vector vertices[nVertices]; */
            "0.0; 0.0; 0.0;,"
            "0.0; 0.0; 1.0;,"
            "0.0; 1.0; 0.0;,"
            "0.0; 1.0; 1.0;,"
            "1.0; 0.0; 0.0;,"
            "1.0; 0.0; 1.0;,"
            "1.0; 1.0; 0.0;,"
            "1.0; 1.0; 1.0;;"
            "6;" /* DWORD nFaces; */
            /* array MeshFace faces[nFaces]; */
            "4; 0, 1, 3, 2;," /* (left side) */
            "4; 2, 3, 7, 6;," /* (top side) */
            "4; 6, 7, 5, 4;," /* (right side) */
            "4; 1, 0, 4, 5;," /* (bottom side) */
            "4; 1, 5, 7, 3;," /* (back side) */
            "4; 0, 2, 6, 4;;" /* (front side) */
            "MeshNormals {"
              "6;" /* DWORD nNormals; */
              /* array Vector normals[nNormals]; */
              "-1.0; 0.0; 0.0;,"
              "0.0; 1.0; 0.0;,"
              "1.0; 0.0; 0.0;,"
              "0.0; -1.0; 0.0;,"
              "0.0; 0.0; 1.0;,"
              "0.0; 0.0; -1.0;;"
              "6;" /* DWORD nFaceNormals; */
              /* array MeshFace faceNormals[nFaceNormals]; */
              "4; 0, 0, 0, 0;,"
              "4; 1, 1, 1, 1;,"
              "4; 2, 2, 2, 2;,"
              "4; 3, 3, 3, 3;,"
              "4; 4, 4, 4, 4;,"
              "4; 5, 5, 5, 5;;"
            "}"
            "TestData {"
                "1;;"
            "}"
            "MeshMaterialList materials {"
              "2;" /* DWORD nMaterials; */
              "6;" /* DWORD nFaceIndexes; */
              /* array DWORD faceIndexes[nFaceIndexes]; */
              "0, 0, 0, 1, 1, 1;;"
              "Material {"
                /* ColorRGBA faceColor; */
                "0.0; 0.0; 1.0; 1.0;;"
                /* FLOAT power; */
                "0.5;"
                /* ColorRGB specularColor; */
                "1.0; 1.0; 1.0;;"
                /* ColorRGB emissiveColor; */
                "0.0; 0.0; 0.0;;"
              "}"
              "Material {"
                /* ColorRGBA faceColor; */
                "1.0; 1.0; 1.0; 1.0;;"
                /* FLOAT power; */
                "1.0;"
                /* ColorRGB specularColor; */
                "1.0; 1.0; 1.0;;"
                /* ColorRGB emissiveColor; */
                "0.0; 0.0; 0.0;;"
                "TextureFilename { \"texture.jpg\"; }"
              "}"
            "}"
            "TestData {"
                "2;;"
            "}"

            "MeshVertexColors {"
              "8;" /* DWORD nVertexColors; */
              /* array IndexedColor vertexColors[nVertexColors]; */
              "0; 0.0; 0.0; 0.0; 0.0;;"
              "1; 0.0; 0.0; 1.0; 0.1;;"
              "2; 0.0; 1.0; 0.0; 0.2;;"
              "3; 0.0; 1.0; 1.0; 0.3;;"
              "4; 1.0; 0.0; 0.0; 0.4;;"
              "5; 1.0; 0.0; 1.0; 0.5;;"
              "6; 1.0; 1.0; 0.0; 0.6;;"
              "7; 1.0; 1.0; 1.0; 0.7;;"
            "}"
            "MeshTextureCoords {"
              "8;" /* DWORD nTextureCoords; */
              /* array Coords2d textureCoords[nTextureCoords]; */
              "0.0; 1.0;,"
              "1.0; 1.0;,"
              "0.0; 0.0;,"
              "1.0; 0.0;,"
              "1.0; 1.0;,"
              "0.0; 1.0;,"
              "1.0; 0.0;,"
              "0.0; 0.0;;"
            "}"
          "}";
    static const struct test_user_data box_xfile_expected_user_data[] =
    {
        { USER_DATA_TYPE_MESH_CHILD, &TID_TestDataGuid, 4, 1, 0, 2},
        { USER_DATA_TYPE_MESH_CHILD, &TID_TestDataGuid, 4, 2, 0, 2},
    };

    static const WORD box_index_buffer[] = {
        0, 1, 3,
        0, 3, 2,
        8, 9, 7,
        8, 7, 6,
        10, 11, 5,
        10, 5, 4,
        12, 13, 14,
        12, 14, 15,
        16, 17, 18,
        16, 18, 19,
        20, 21, 22,
        20, 22, 23,
    };
    static const struct {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        D3DCOLOR diffuse;
        D3DXVECTOR2 tex_coords;
    } box_vertex_buffer[] = {
        {{0.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}, 0x00000000, {0.0, 1.0}},
        {{0.0, 0.0, 1.0}, {-1.0, 0.0, 0.0}, 0x1a0000ff, {1.0, 1.0}},
        {{0.0, 1.0, 0.0}, {-1.0, 0.0, 0.0}, 0x3300ff00, {0.0, 0.0}},
        {{0.0, 1.0, 1.0}, {-1.0, 0.0, 0.0}, 0x4d00ffff, {1.0, 0.0}},
        {{1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0x66ff0000, {1.0, 1.0}},
        {{1.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, 0x80ff00ff, {0.0, 1.0}},
        {{1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, 0x99ffff00, {1.0, 0.0}},
        {{1.0, 1.0, 1.0}, {0.0, 1.0, 0.0}, 0xb3ffffff, {0.0, 0.0}},
        {{0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, 0x3300ff00, {0.0, 0.0}},
        {{0.0, 1.0, 1.0}, {0.0, 1.0, 0.0}, 0x4d00ffff, {1.0, 0.0}},
        {{1.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, 0x99ffff00, {1.0, 0.0}},
        {{1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}, 0xb3ffffff, {0.0, 0.0}},
        {{0.0, 0.0, 1.0}, {0.0, -1.0, 0.0}, 0x1a0000ff, {1.0, 1.0}},
        {{0.0, 0.0, 0.0}, {0.0, -1.0, 0.0}, 0x00000000, {0.0, 1.0}},
        {{1.0, 0.0, 0.0}, {0.0, -1.0, 0.0}, 0x66ff0000, {1.0, 1.0}},
        {{1.0, 0.0, 1.0}, {0.0, -1.0, 0.0}, 0x80ff00ff, {0.0, 1.0}},
        {{0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, 0x1a0000ff, {1.0, 1.0}},
        {{1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, 0x80ff00ff, {0.0, 1.0}},
        {{1.0, 1.0, 1.0}, {0.0, 0.0, 1.0}, 0xb3ffffff, {0.0, 0.0}},
        {{0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}, 0x4d00ffff, {1.0, 0.0}},
        {{0.0, 0.0, 0.0}, {0.0, 0.0, -1.0}, 0x00000000, {0.0, 1.0}},
        {{0.0, 1.0, 0.0}, {0.0, 0.0, -1.0}, 0x3300ff00, {0.0, 0.0}},
        {{1.0, 1.0, 0.0}, {0.0, 0.0, -1.0}, 0x99ffff00, {1.0, 0.0}},
        {{1.0, 0.0, 0.0}, {0.0, 0.0, -1.0}, 0x66ff0000, {1.0, 1.0}},
    };
    static const D3DXMATERIAL box_materials[] = {
        {
            {
                {0.0, 0.0, 1.0, 1.0}, /* Diffuse */
                {0.0, 0.0, 0.0, 1.0}, /* Ambient */
                {1.0, 1.0, 1.0, 1.0}, /* Specular */
                {0.0, 0.0, 0.0, 1.0}, /* Emissive */
                0.5, /* Power */
            },
            NULL, /* pTextureFilename */
        },
        {
            {
                {1.0, 1.0, 1.0, 1.0}, /* Diffuse */
                {0.0, 0.0, 0.0, 1.0}, /* Ambient */
                {1.0, 1.0, 1.0, 1.0}, /* Specular */
                {0.0, 0.0, 0.0, 1.0}, /* Emissive */
                1.0, /* Power */
            },
            (char *)"texture.jpg", /* pTextureFilename */
        },
    };
    static const char box_anim_xfile[] =
        "xof 0303txt 0032"
        "Mesh CubeMesh {"
        "8;" /* DWORD nVertices; */
        /* array Vector vertices[nVertices]; */
        "0.0; 0.0; 0.0;,"
        "0.0; 0.0; 1.0;,"
        "0.0; 1.0; 0.0;,"
        "0.0; 1.0; 1.0;,"
        "1.0; 0.0; 0.0;,"
        "1.0; 0.0; 1.0;,"
        "1.0; 1.0; 0.0;,"
        "1.0; 1.0; 1.0;;"
        "6;" /* DWORD nFaces; */
        /* array MeshFace faces[nFaces]; */
        "4; 0, 1, 3, 2;," /* left side */
        "4; 2, 3, 7, 6;," /* top side */
        "4; 6, 7, 5, 4;," /* right side */
        "4; 1, 0, 4, 5;," /* bottom side */
        "4; 1, 5, 7, 3;," /* back side */
        "4; 0, 2, 6, 4;;" /* front side */
        "MeshNormals {"
        "6;" /* DWORD nNormals; */
        /* array Vector normals[nNormals]; */
        "-1.0; 0.0; 0.0;,"
        "0.0; 1.0; 0.0;,"
        "1.0; 0.0; 0.0;,"
        "0.0; -1.0; 0.0;,"
        "0.0; 0.0; 1.0;,"
        "0.0; 0.0; -1.0;;"
        "6;" /* DWORD nFaceNormals; */
        /* array MeshFace faceNormals[nFaceNormals]; */
        "4; 0, 0, 0, 0;,"
        "4; 1, 1, 1, 1;,"
        "4; 2, 2, 2, 2;,"
        "4; 3, 3, 3, 3;,"
        "4; 4, 4, 4, 4;,"
        "4; 5, 5, 5, 5;;"
        "}"
        "MeshMaterialList materials {"
        "2;" /* DWORD nMaterials; */
        "6;" /* DWORD nFaceIndexes; */
        /* array DWORD faceIndexes[nFaceIndexes]; */
        "0, 0, 0, 1, 1, 1;;"
        "Material {"
        /* ColorRGBA faceColor; */
        "0.0; 0.0; 1.0; 1.0;;"
        /* FLOAT power; */
        "0.5;"
        /* ColorRGB specularColor; */
        "1.0; 1.0; 1.0;;"
        /* ColorRGB emissiveColor; */
        "0.0; 0.0; 0.0;;"
        "}"
        "Material {"
        /* ColorRGBA faceColor; */
        "1.0; 1.0; 1.0; 1.0;;"
        /* FLOAT power; */
        "1.0;"
        /* ColorRGB specularColor; */
        "1.0; 1.0; 1.0;;"
        /* ColorRGB emissiveColor; */
        "0.0; 0.0; 0.0;;"
        "TextureFilename { \"texture.jpg\"; }"
        "}"
        "}"
        "MeshVertexColors {"
        "8;" /* DWORD nVertexColors; */
        /* array IndexedColor vertexColors[nVertexColors]; */
        "0; 0.0; 0.0; 0.0; 0.0;;"
        "1; 0.0; 0.0; 1.0; 0.1;;"
        "2; 0.0; 1.0; 0.0; 0.2;;"
        "3; 0.0; 1.0; 1.0; 0.3;;"
        "4; 1.0; 0.0; 0.0; 0.4;;"
        "5; 1.0; 0.0; 1.0; 0.5;;"
        "6; 1.0; 1.0; 0.0; 0.6;;"
        "7; 1.0; 1.0; 1.0; 0.7;;"
        "}"
        "MeshTextureCoords {"
        "8;" /* DWORD nTextureCoords; */
        /* array Coords2d textureCoords[nTextureCoords]; */
        "0.0; 1.0;,"
        "1.0; 1.0;,"
        "0.0; 0.0;,"
        "1.0; 0.0;,"
        "1.0; 1.0;,"
        "0.0; 1.0;,"
        "1.0; 0.0;,"
        "0.0; 0.0;;"
        "}"
        "}"
        "Frame CubeFrame {"
        "FrameTransformMatrix {"
        /* Matrix4x4 frameMatrix; */
        "1.0, 0.0, 0.0, 0.0,"
        "0.0, 1.0, 0.0, 0.0,"
        "0.0, 0.0, 1.0, 0.0,"
        "0.0, 0.0, 0.0, 1.0;;"
        "}"
        "{CubeMesh}"
        "}"
        "AnimationSet AnimationSet0 {"
        "Animation Animation0 {"
        "{CubeFrame}"
        "AnimationKey {"
        "2;" /* DWORD keyType; */
        "9;" /* DWORD nKeys; */
        /* array TimedFloatKeys keys[nKeys]; */
        "10; 3; -100.0, 0.0, 0.0;;,"
        "20; 3; -75.0,  0.0, 0.0;;,"
        "30; 3; -50.0,  0.0, 0.0;;,"
        "40; 3; -25.5,  0.0, 0.0;;,"
        "50; 3; 0.0,    0.0, 0.0;;,"
        "60; 3; 25.5,   0.0, 0.0;;,"
        "70; 3; 50.0,   0.0, 0.0;;,"
        "80; 3; 75.5,   0.0, 0.0;;,"
        "90; 3; 100.0,  0.0, 0.0;;;"
        "}"
        "}"
        "}";

    const DWORD box_fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
    /*________________________*/
    static const D3DXMATERIAL default_materials[] = {
        {
            {
                {0.5, 0.5, 0.5, 0.0}, /* Diffuse */
                {0.0, 0.0, 0.0, 0.0}, /* Ambient */
                {0.5, 0.5, 0.5, 0.0}, /* Specular */
                {0.0, 0.0, 0.0, 0.0}, /* Emissive */
                0.0, /* Power */
            },
            NULL, /* pTextureFilename */
        }
    };
    HRESULT hr;
    IDirect3DDevice9 *device = NULL;
    ID3DXMesh *mesh = NULL;
    D3DXFRAME *frame_hier = NULL;
    D3DXMATRIX transform;
    struct test_context *test_context;
    ID3DXAnimationController *controller;
    D3DXMESHCONTAINER *container;
    unsigned int i;
    struct test_load_user_data load_user_data;

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXLoadMeshHierarchyFromXInMemory(NULL, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, 0,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, NULL, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, NULL, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(empty_xfile, sizeof(empty_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == E_FAIL, "Expected E_FAIL, got %#lx\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    container = frame_hier->pMeshContainer;

    ok(frame_hier->Name == NULL, "Expected NULL, got '%s'\n", frame_hier->Name);
    D3DXMatrixIdentity(&transform);
    check_matrix(&frame_hier->TransformationMatrix, &transform);

    ok(!strcmp(container->Name, ""), "Expected '', got '%s'\n", container->Name);
    ok(container->MeshData.Type == D3DXMESHTYPE_MESH, "Expected %d, got %d\n",
            D3DXMESHTYPE_MESH, container->MeshData.Type);
    mesh = container->MeshData.pMesh;
    check_vertex_buffer(mesh, simple_vertex_buffer, ARRAY_SIZE(simple_vertex_buffer), simple_fvf);
    check_index_buffer(mesh, simple_index_buffer, ARRAY_SIZE(simple_index_buffer), sizeof(*simple_index_buffer));
    check_materials(container->pMaterials, container->NumMaterials, NULL, 0);
    check_generated_effects(container->pMaterials, container->NumMaterials, container->pEffects);
    check_generated_adjacency(mesh, container->pAdjacency, 0.0f);
    hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    frame_hier = NULL;

    controller = (ID3DXAnimationController *)0xdeadbeef;
    hr = D3DXLoadMeshHierarchyFromXInMemory(box_anim_xfile, sizeof(box_anim_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, &controller);
    todo_wine ok(hr == D3D_OK, "Expected D3D_OK, got %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(controller != NULL, "Animation Controller NULL.\n");

        hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#lx.\n", hr);
        if (controller)
            controller->lpVtbl->Release(controller);

        frame_hier = NULL;
    }

    controller = (ID3DXAnimationController *)0xdeadbeef;
    hr = D3DXLoadMeshHierarchyFromXInMemory(box_xfile, sizeof(box_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, &controller);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    container = frame_hier->pMeshContainer;

    ok(!controller, "Animation Controller returned.\n");
    ok(frame_hier->Name == NULL, "Expected NULL, got '%s'\n", frame_hier->Name);
    D3DXMatrixIdentity(&transform);
    check_matrix(&frame_hier->TransformationMatrix, &transform);

    ok(!strcmp(container->Name, ""), "Expected '', got '%s'\n", container->Name);
    ok(container->MeshData.Type == D3DXMESHTYPE_MESH, "Expected %d, got %d\n",
            D3DXMESHTYPE_MESH, container->MeshData.Type);
    mesh = container->MeshData.pMesh;
    check_vertex_buffer(mesh, box_vertex_buffer, ARRAY_SIZE(box_vertex_buffer), box_fvf);
    check_index_buffer(mesh, box_index_buffer, ARRAY_SIZE(box_index_buffer), sizeof(*box_index_buffer));
    check_materials(container->pMaterials, container->NumMaterials, box_materials, ARRAY_SIZE(box_materials));
    check_generated_effects(container->pMaterials, container->NumMaterials, container->pEffects);
    check_generated_adjacency(mesh, container->pAdjacency, 0.0f);
    hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    frame_hier = NULL;

    init_load_user_data(&load_user_data);
    hr = D3DXLoadMeshHierarchyFromXInMemory(box_xfile, sizeof(box_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, &load_user_data.iface, &frame_hier, &controller);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    winetest_push_context("box_xfile");
    check_user_data(&load_user_data, ARRAY_SIZE(box_xfile_expected_user_data), box_xfile_expected_user_data);
    winetest_pop_context();
    hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    frame_hier = NULL;

    hr = D3DXLoadMeshHierarchyFromXInMemory(framed_xfile, sizeof(framed_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    container = frame_hier->pMeshContainer;

    ok(!strcmp(frame_hier->Name, ""), "Expected '', got '%s'\n", frame_hier->Name);
    /* last frame transform replaces the first */
    D3DXMatrixIdentity(&transform);
    transform.m[3][2] = 3.0;
    check_matrix(&frame_hier->TransformationMatrix, &transform);

    for (i = 0; i < 3; ++i)
    {
        ok(!strcmp(container->Name, ""), "Expected '', got '%s'\n", container->Name);
        ok(container->MeshData.Type == D3DXMESHTYPE_MESH, "Expected %d, got %d\n",
                D3DXMESHTYPE_MESH, container->MeshData.Type);
        mesh = container->MeshData.pMesh;
        check_vertex_buffer(mesh, framed_vertex_buffers[i], ARRAY_SIZE(framed_vertex_buffers[0]), framed_fvf);
        check_index_buffer(mesh, framed_index_buffer, ARRAY_SIZE(framed_index_buffer), sizeof(*framed_index_buffer));
        check_materials(container->pMaterials, container->NumMaterials, NULL, 0);
        check_generated_effects(container->pMaterials, container->NumMaterials, container->pEffects);
        check_generated_adjacency(mesh, container->pAdjacency, 0.0f);
        container = container->pNextMeshContainer;
    }
    ok(container == NULL, "Expected NULL, got %p\n", container);
    hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    frame_hier = NULL;

    init_load_user_data(&load_user_data);
    hr = D3DXLoadMeshHierarchyFromXInMemory(framed_xfile, sizeof(framed_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, &load_user_data.iface, &frame_hier, NULL);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    winetest_push_context("framed_xfile");
    check_user_data(&load_user_data, ARRAY_SIZE(framed_xfile_expected_user_data), framed_xfile_expected_user_data);
    winetest_pop_context();
    frame_hier = NULL;

    hr = D3DXLoadMeshHierarchyFromXInMemory(framed_xfile_empty, sizeof(framed_xfile_empty) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    container = frame_hier->pMeshContainer;
    ok(!strcmp(frame_hier->Name, "Box01"), "Unexpected name %s.\n", debugstr_a(frame_hier->Name));
    ok(!container, "Unexpected container %p.\n", container);

    hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    frame_hier = NULL;

    hr = D3DXLoadMeshFromXInMemory(NULL, 0, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshFromXInMemory(NULL, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, 0, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   NULL, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXLoadMeshFromXInMemory(empty_xfile, sizeof(empty_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == E_FAIL, "Expected E_FAIL, got %#lx\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    IUnknown_Release(mesh);

    test_LoadMeshFromX(device, simple_xfile, simple_vertex_buffer, simple_fvf, simple_index_buffer, default_materials, TRUE);
    test_LoadMeshFromX(device, box_xfile, box_vertex_buffer, box_fvf, box_index_buffer, box_materials, TRUE);
    test_LoadMeshFromX(device, framed_xfile, merged_vertex_buffer, framed_fvf, merged_index_buffer, default_materials, FALSE);

    free_test_context(test_context);
}

static BOOL compute_box(struct mesh *mesh, float width, float height, float depth)
{
    unsigned int i, face;
    static const D3DXVECTOR3 unit_box[] =
    {
        {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f},
        {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}
    };
    static const D3DXVECTOR3 normals[] =
    {
        {-1.0f,  0.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f,  0.0f},
        { 0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, -1.0f}
    };

    if (!new_mesh(mesh, 24, 12))
    {
        return FALSE;
    }

    width /= 2.0f;
    height /= 2.0f;
    depth /= 2.0f;

    for (i = 0; i < 24; i++)
    {
        mesh->vertices[i].position.x = width * unit_box[i].x;
        mesh->vertices[i].position.y = height * unit_box[i].y;
        mesh->vertices[i].position.z = depth * unit_box[i].z;
        mesh->vertices[i].normal.x = normals[i / 4].x;
        mesh->vertices[i].normal.y = normals[i / 4].y;
        mesh->vertices[i].normal.z = normals[i / 4].z;
    }

    face = 0;
    for (i = 0; i < 12; i++)
    {
        mesh->faces[i][0] = face++;
        mesh->faces[i][1] = face++;
        mesh->faces[i][2] = (i % 2) ? face - 4 : face;
    }

    return TRUE;
}

static void test_box(IDirect3DDevice9 *device, float width, float height, float depth)
{
    HRESULT hr;
    ID3DXMesh *box;
    struct mesh mesh;
    char name[256];

    hr = D3DXCreateBox(device, width, height, depth, &box, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create box\n");
        return;
    }

    if (!compute_box(&mesh, width, height, depth))
    {
        skip("Couldn't create mesh\n");
        box->lpVtbl->Release(box);
        return;
    }

    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    sprintf(name, "box (%g, %g, %g)", width, height, depth);
    compare_mesh(name, box, &mesh);

    free_mesh(&mesh);

    box->lpVtbl->Release(box);
}
static void D3DXCreateBoxTest(void)
{
    HRESULT hr;
    IDirect3DDevice9* device;
    ID3DXMesh* box;
    ID3DXBuffer* ppBuffer;
    DWORD *buffer;
    static const DWORD adjacency[36]=
        {6, 9, 1, 2, 10, 0,
         1, 9, 3, 4, 10, 2,
         3, 8, 5, 7, 11, 4,
         0, 11, 7, 5, 8, 6,
         7, 4, 9, 2, 0, 8,
         1, 3, 11, 5, 6, 10};
    unsigned int i;
    struct test_context *test_context;

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateBox(device,2.0f,20.0f,4.9f,NULL, &ppBuffer);
    ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    hr = D3DXCreateBox(NULL,22.0f,20.0f,4.9f,&box, &ppBuffer);
    ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    hr = D3DXCreateBox(device,-2.0f,20.0f,4.9f,&box, &ppBuffer);
    ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    hr = D3DXCreateBox(device,22.0f,-20.0f,4.9f,&box, &ppBuffer);
    ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    hr = D3DXCreateBox(device,22.0f,20.0f,-4.9f,&box, &ppBuffer);
    ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    ppBuffer = NULL;
    hr = D3DXCreateBox(device,10.9f,20.0f,4.9f,&box, &ppBuffer);
    ok(hr==D3D_OK, "Expected D3D_OK, received %#lx\n", hr);

    buffer = ID3DXBuffer_GetBufferPointer(ppBuffer);
    for(i=0; i<36; i++)
        ok(adjacency[i]==buffer[i], "expected adjacency %d: %#lx, received %#lx\n",i,adjacency[i], buffer[i]);

    box->lpVtbl->Release(box);
    ID3DXBuffer_Release(ppBuffer);

    test_box(device, 10.9f, 20.0f, 4.9f);

    free_test_context(test_context);
}

static BOOL compute_polygon(struct mesh *mesh, float length, unsigned int sides)
{
    unsigned int i;
    float angle, scale;

    if (!new_mesh(mesh, sides + 1, sides))
        return FALSE;

    angle = D3DX_PI / sides;
    scale = 0.5f * length / sinf(angle);
    angle *= 2.0f;

    mesh->vertices[0].position.x = 0.0f;
    mesh->vertices[0].position.y = 0.0f;
    mesh->vertices[0].position.z = 0.0f;
    mesh->vertices[0].normal.x = 0.0f;
    mesh->vertices[0].normal.y = 0.0f;
    mesh->vertices[0].normal.z = 1.0f;

    for (i = 0; i < sides; ++i)
    {
        mesh->vertices[i + 1].position.x = cosf(angle * i) * scale;
        mesh->vertices[i + 1].position.y = sinf(angle * i) * scale;
        mesh->vertices[i + 1].position.z = 0.0f;
        mesh->vertices[i + 1].normal.x = 0.0f;
        mesh->vertices[i + 1].normal.y = 0.0f;
        mesh->vertices[i + 1].normal.z = 1.0f;

        mesh->faces[i][0] = 0;
        mesh->faces[i][1] = i + 1;
        mesh->faces[i][2] = i + 2;
    }

    mesh->faces[sides - 1][2] = 1;

    return TRUE;
}

static void test_polygon(IDirect3DDevice9 *device, float length, unsigned int sides)
{
    HRESULT hr;
    ID3DXMesh *polygon;
    struct mesh mesh;
    char name[64];

    hr = D3DXCreatePolygon(device, length, sides, &polygon, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create polygon\n");
        return;
    }

    if (!compute_polygon(&mesh, length, sides))
    {
        skip("Couldn't create mesh\n");
        polygon->lpVtbl->Release(polygon);
        return;
    }

    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    sprintf(name, "polygon (%g, %u)", length, sides);
    compare_mesh(name, polygon, &mesh);

    free_mesh(&mesh);

    polygon->lpVtbl->Release(polygon);
}

static void D3DXCreatePolygonTest(void)
{
    HRESULT hr;
    IDirect3DDevice9 *device;
    ID3DXMesh *polygon;
    ID3DXBuffer *adjacency;
    DWORD (*buffer)[3], buffer_size;
    unsigned int i;
    struct test_context *test_context;

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreatePolygon(device, 2.0f, 11, NULL, &adjacency);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    hr = D3DXCreatePolygon(NULL, 2.0f, 11, &polygon, &adjacency);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    hr = D3DXCreatePolygon(device, -2.0f, 11, &polygon, &adjacency);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    polygon = (void *)0xdeadbeef;
    adjacency = (void *)0xdeadbeef;
    hr = D3DXCreatePolygon(device, 2.0f, 0, &polygon, &adjacency);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);
    ok(polygon == (void *)0xdeadbeef, "Polygon was changed to %p\n", polygon);
    ok(adjacency == (void *)0xdeadbeef, "Adjacency was changed to %p\n", adjacency);

    hr = D3DXCreatePolygon(device, 2.0f, 2, &polygon, &adjacency);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#lx\n", hr);

    adjacency = NULL;
    hr = D3DXCreatePolygon(device, 3.0f, 11, &polygon, &adjacency);
    ok(hr == D3D_OK, "Expected D3D_OK, received %#lx\n", hr);

    buffer_size = ID3DXBuffer_GetBufferSize(adjacency);
    ok(buffer_size == 33 * sizeof(DWORD), "Wrong adjacency buffer size %lu\n", buffer_size);

    buffer = ID3DXBuffer_GetBufferPointer(adjacency);
    for (i = 0; i < 11; ++i)
    {
        ok(buffer[i][0] == (i + 10) % 11, "Wrong adjacency[%d][0] = %lu\n", i, buffer[i][0]);
        ok(buffer[i][1] == ~0U, "Wrong adjacency[%d][1] = %lu\n", i, buffer[i][1]);
        ok(buffer[i][2] == (i + 1) % 11, "Wrong adjacency[%d][2] = %lu\n", i, buffer[i][2]);
    }

    polygon->lpVtbl->Release(polygon);
    ID3DXBuffer_Release(adjacency);

    test_polygon(device, 2.0f, 3);
    test_polygon(device, 10.0f, 3);
    test_polygon(device, 10.0f, 5);
    test_polygon(device, 10.0f, 10);
    test_polygon(device, 20.0f, 10);
    test_polygon(device, 20.0f, 32000);

    free_test_context(test_context);
}

struct sincos_table
{
    float *sin;
    float *cos;
};

static void free_sincos_table(struct sincos_table *sincos_table)
{
    free(sincos_table->cos);
    free(sincos_table->sin);
}

/* pre compute sine and cosine tables; caller must free */
static BOOL compute_sincos_table(struct sincos_table *sincos_table, float angle_start, float angle_step, int n)
{
    float angle;
    int i;

    sincos_table->sin = malloc(n * sizeof(*sincos_table->sin));
    if (!sincos_table->sin)
    {
        return FALSE;
    }
    sincos_table->cos = malloc(n * sizeof(*sincos_table->cos));
    if (!sincos_table->cos)
    {
        free(sincos_table->sin);
        return FALSE;
    }

    angle = angle_start;
    for (i = 0; i < n; i++)
    {
        sincos_table->sin[i] = sin(angle);
        sincos_table->cos[i] = cos(angle);
        angle += angle_step;
    }

    return TRUE;
}

static WORD vertex_index(UINT slices, int slice, int stack)
{
    return stack*slices+slice+1;
}

/* slices = subdivisions along xy plane, stacks = subdivisions along z axis */
static BOOL compute_sphere(struct mesh *mesh, FLOAT radius, UINT slices, UINT stacks)
{
    float theta_step, theta_start;
    struct sincos_table theta;
    float phi_step, phi_start;
    struct sincos_table phi;
    DWORD number_of_vertices, number_of_faces;
    DWORD vertex, face;
    int slice, stack;

    /* theta = angle on xy plane wrt x axis */
    theta_step = D3DX_PI / stacks;
    theta_start = theta_step;

    /* phi = angle on xz plane wrt z axis */
    phi_step = -2 * D3DX_PI / slices;
    phi_start = D3DX_PI / 2;

    if (!compute_sincos_table(&theta, theta_start, theta_step, stacks))
    {
        return FALSE;
    }
    if (!compute_sincos_table(&phi, phi_start, phi_step, slices))
    {
        free_sincos_table(&theta);
        return FALSE;
    }

    number_of_vertices = 2 + slices * (stacks-1);
    number_of_faces = 2 * slices + (stacks - 2) * (2 * slices);

    if (!new_mesh(mesh, number_of_vertices, number_of_faces))
    {
        free_sincos_table(&phi);
        free_sincos_table(&theta);
        return FALSE;
    }

    vertex = 0;
    face = 0;

    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = 1.0f;
    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex].position.z = radius;
    vertex++;

    for (stack = 0; stack < stacks - 1; stack++)
    {
        for (slice = 0; slice < slices; slice++)
        {
            mesh->vertices[vertex].normal.x = theta.sin[stack] * phi.cos[slice];
            mesh->vertices[vertex].normal.y = theta.sin[stack] * phi.sin[slice];
            mesh->vertices[vertex].normal.z = theta.cos[stack];
            mesh->vertices[vertex].position.x = radius * theta.sin[stack] * phi.cos[slice];
            mesh->vertices[vertex].position.y = radius * theta.sin[stack] * phi.sin[slice];
            mesh->vertices[vertex].position.z = radius * theta.cos[stack];
            vertex++;

            if (slice > 0)
            {
                if (stack == 0)
                {
                    /* top stack is triangle fan */
                    mesh->faces[face][0] = 0;
                    mesh->faces[face][1] = slice + 1;
                    mesh->faces[face][2] = slice;
                    face++;
                }
                else
                {
                    /* stacks in between top and bottom are quad strips */
                    mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
                    mesh->faces[face][1] = vertex_index(slices, slice, stack-1);
                    mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
                    face++;

                    mesh->faces[face][0] = vertex_index(slices, slice, stack-1);
                    mesh->faces[face][1] = vertex_index(slices, slice, stack);
                    mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
                    face++;
                }
            }
        }

        if (stack == 0)
        {
            mesh->faces[face][0] = 0;
            mesh->faces[face][1] = 1;
            mesh->faces[face][2] = slice;
            face++;
        }
        else
        {
            mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
            mesh->faces[face][1] = vertex_index(slices, 0, stack-1);
            mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
            face++;

            mesh->faces[face][0] = vertex_index(slices, 0, stack-1);
            mesh->faces[face][1] = vertex_index(slices, 0, stack);
            mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
            face++;
        }
    }

    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex].position.z = -radius;
    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = -1.0f;

    /* bottom stack is triangle fan */
    for (slice = 1; slice < slices; slice++)
    {
        mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
        mesh->faces[face][1] = vertex_index(slices, slice, stack-1);
        mesh->faces[face][2] = vertex;
        face++;
    }

    mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
    mesh->faces[face][1] = vertex_index(slices, 0, stack-1);
    mesh->faces[face][2] = vertex;

    free_sincos_table(&phi);
    free_sincos_table(&theta);

    return TRUE;
}

static void test_sphere(IDirect3DDevice9 *device, FLOAT radius, UINT slices, UINT stacks)
{
    HRESULT hr;
    ID3DXMesh *sphere;
    struct mesh mesh;
    char name[256];

    hr = D3DXCreateSphere(device, radius, slices, stacks, &sphere, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create sphere\n");
        return;
    }

    if (!compute_sphere(&mesh, radius, slices, stacks))
    {
        skip("Couldn't create mesh\n");
        sphere->lpVtbl->Release(sphere);
        return;
    }

    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    sprintf(name, "sphere (%g, %u, %u)", radius, slices, stacks);
    compare_mesh(name, sphere, &mesh);

    free_mesh(&mesh);

    sphere->lpVtbl->Release(sphere);
}

static void D3DXCreateSphereTest(void)
{
    HRESULT hr;
    IDirect3DDevice9* device;
    ID3DXMesh* sphere = NULL;
    struct test_context *test_context;

    hr = D3DXCreateSphere(NULL, 0.0f, 0, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.1f, 0, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.0f, 1, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.0f, 0, 1, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateSphere(device, 1.0f, 1, 1, &sphere, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, 1.0f, 2, 1, &sphere, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, 1.0f, 1, 2, &sphere, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, -0.1f, 1, 2, &sphere, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    test_sphere(device, 0.0f, 2, 2);
    test_sphere(device, 1.0f, 2, 2);
    test_sphere(device, 1.0f, 3, 2);
    test_sphere(device, 1.0f, 4, 4);
    test_sphere(device, 1.0f, 3, 4);
    test_sphere(device, 5.0f, 6, 7);
    test_sphere(device, 10.0f, 11, 12);

    free_test_context(test_context);
}

static BOOL compute_cylinder(struct mesh *mesh, FLOAT radius1, FLOAT radius2, FLOAT length, UINT slices, UINT stacks)
{
    float theta_step, theta_start;
    struct sincos_table theta;
    FLOAT delta_radius, radius, radius_step;
    FLOAT z, z_step, z_normal;
    DWORD number_of_vertices, number_of_faces;
    DWORD vertex, face;
    int slice, stack;

    /* theta = angle on xy plane wrt x axis */
    theta_step = -2 * D3DX_PI / slices;
    theta_start = D3DX_PI / 2;

    if (!compute_sincos_table(&theta, theta_start, theta_step, slices))
    {
        return FALSE;
    }

    number_of_vertices = 2 + (slices * (3 + stacks));
    number_of_faces = 2 * slices + stacks * (2 * slices);

    if (!new_mesh(mesh, number_of_vertices, number_of_faces))
    {
        free_sincos_table(&theta);
        return FALSE;
    }

    vertex = 0;
    face = 0;

    delta_radius = radius1 - radius2;
    radius = radius1;
    radius_step = delta_radius / stacks;

    z = -length / 2;
    z_step = length / stacks;
    z_normal = delta_radius / length;
    if (isnan(z_normal))
    {
        z_normal = 0.0f;
    }

    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = -1.0f;
    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex++].position.z = z;

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        mesh->vertices[vertex].normal.x = 0.0f;
        mesh->vertices[vertex].normal.y = 0.0f;
        mesh->vertices[vertex].normal.z = -1.0f;
        mesh->vertices[vertex].position.x = radius * theta.cos[slice];
        mesh->vertices[vertex].position.y = radius * theta.sin[slice];
        mesh->vertices[vertex].position.z = z;

        if (slice > 0)
        {
            mesh->faces[face][0] = 0;
            mesh->faces[face][1] = slice;
            mesh->faces[face++][2] = slice + 1;
        }
    }

    mesh->faces[face][0] = 0;
    mesh->faces[face][1] = slice;
    mesh->faces[face++][2] = 1;

    for (stack = 1; stack <= stacks+1; stack++)
    {
        for (slice = 0; slice < slices; slice++, vertex++)
        {
            mesh->vertices[vertex].normal.x = theta.cos[slice];
            mesh->vertices[vertex].normal.y = theta.sin[slice];
            mesh->vertices[vertex].normal.z = z_normal;
            D3DXVec3Normalize(&mesh->vertices[vertex].normal, &mesh->vertices[vertex].normal);
            mesh->vertices[vertex].position.x = radius * theta.cos[slice];
            mesh->vertices[vertex].position.y = radius * theta.sin[slice];
            mesh->vertices[vertex].position.z = z;

            if (stack > 1 && slice > 0)
            {
                mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
                mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
                mesh->faces[face++][2] = vertex_index(slices, slice, stack-1);

                mesh->faces[face][0] = vertex_index(slices, slice, stack-1);
                mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
                mesh->faces[face++][2] = vertex_index(slices, slice, stack);
            }
        }

        if (stack > 1)
        {
            mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
            mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
            mesh->faces[face++][2] = vertex_index(slices, 0, stack-1);

            mesh->faces[face][0] = vertex_index(slices, 0, stack-1);
            mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
            mesh->faces[face++][2] = vertex_index(slices, 0, stack);
        }

        if (stack < stacks + 1)
        {
            z += z_step;
            radius -= radius_step;
        }
    }

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        mesh->vertices[vertex].normal.x = 0.0f;
        mesh->vertices[vertex].normal.y = 0.0f;
        mesh->vertices[vertex].normal.z = 1.0f;
        mesh->vertices[vertex].position.x = radius * theta.cos[slice];
        mesh->vertices[vertex].position.y = radius * theta.sin[slice];
        mesh->vertices[vertex].position.z = z;

        if (slice > 0)
        {
            mesh->faces[face][0] = vertex_index(slices, slice-1, stack);
            mesh->faces[face][1] = number_of_vertices - 1;
            mesh->faces[face++][2] = vertex_index(slices, slice, stack);
        }
    }

    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex].position.z = z;
    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = 1.0f;

    mesh->faces[face][0] = vertex_index(slices, slice-1, stack);
    mesh->faces[face][1] = number_of_vertices - 1;
    mesh->faces[face][2] = vertex_index(slices, 0, stack);

    free_sincos_table(&theta);

    return TRUE;
}

static void test_cylinder(IDirect3DDevice9 *device, FLOAT radius1, FLOAT radius2, FLOAT length, UINT slices, UINT stacks)
{
    HRESULT hr;
    ID3DXMesh *cylinder;
    struct mesh mesh;
    char name[256];

    hr = D3DXCreateCylinder(device, radius1, radius2, length, slices, stacks, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create cylinder\n");
        return;
    }

    if (!compute_cylinder(&mesh, radius1, radius2, length, slices, stacks))
    {
        skip("Couldn't create mesh\n");
        cylinder->lpVtbl->Release(cylinder);
        return;
    }

    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    sprintf(name, "cylinder (%g, %g, %g, %u, %u)", radius1, radius2, length, slices, stacks);
    compare_mesh(name, cylinder, &mesh);

    free_mesh(&mesh);

    cylinder->lpVtbl->Release(cylinder);
}

static void D3DXCreateCylinderTest(void)
{
    HRESULT hr;
    IDirect3DDevice9* device;
    ID3DXMesh* cylinder = NULL;
    struct test_context *test_context;

    hr = D3DXCreateCylinder(NULL, 0.0f, 0.0f, 0.0f, 0, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(NULL, 1.0f, 1.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateCylinder(device, -0.1f, 1.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 0.0f, 1.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n",hr);

    if (SUCCEEDED(hr) && cylinder)
    {
        cylinder->lpVtbl->Release(cylinder);
    }

    hr = D3DXCreateCylinder(device, 1.0f, -0.1f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 1.0f, 0.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n",hr);

    if (SUCCEEDED(hr) && cylinder)
    {
        cylinder->lpVtbl->Release(cylinder);
    }

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, -0.1f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    /* Test with length == 0.0f succeeds */
    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 0.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n",hr);

    if (SUCCEEDED(hr) && cylinder)
    {
        cylinder->lpVtbl->Release(cylinder);
    }

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 1.0f, 1, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 1.0f, 2, 0, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 1.0f, 2, 1, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    test_cylinder(device, 0.0f, 0.0f, 0.0f, 2, 1);
    test_cylinder(device, 1.0f, 1.0f, 1.0f, 2, 1);
    test_cylinder(device, 1.0f, 1.0f, 2.0f, 3, 4);
    test_cylinder(device, 3.0f, 2.0f, 4.0f, 3, 4);
    test_cylinder(device, 2.0f, 3.0f, 4.0f, 3, 4);
    test_cylinder(device, 3.0f, 4.0f, 5.0f, 11, 20);

    free_test_context(test_context);
}

static BOOL compute_torus(struct mesh *mesh, float innerradius, float outerradius, UINT sides, UINT rings)
{
    float phi, phi_step, sin_phi, cos_phi;
    float theta, theta_step, sin_theta, cos_theta;
    unsigned int numvert, numfaces, i, j;

    numvert = sides * rings;
    numfaces = numvert * 2;

    if (!new_mesh(mesh, numvert, numfaces))
        return FALSE;

    phi_step = D3DX_PI / sides * 2.0f;
    theta_step = D3DX_PI / rings * -2.0f;

    theta = 0.0f;

    for (i = 0; i < rings; ++i)
    {
        phi = 0.0f;

        cos_theta = cosf(theta);
        sin_theta = sinf(theta);

        for (j = 0; j < sides; ++j)
        {
            sin_phi = sinf(phi);
            cos_phi = cosf(phi);

            mesh->vertices[i * sides + j].position.x = (innerradius * cos_phi + outerradius) * cos_theta;
            mesh->vertices[i * sides + j].position.y = (innerradius * cos_phi + outerradius) * sin_theta;
            mesh->vertices[i * sides + j].position.z = innerradius * sin_phi;
            mesh->vertices[i * sides + j].normal.x = cos_phi * cos_theta;
            mesh->vertices[i * sides + j].normal.y = cos_phi * sin_theta;
            mesh->vertices[i * sides + j].normal.z = sin_phi;

            phi += phi_step;
        }

        theta += theta_step;
    }

    for (i = 0; i < numfaces - sides * 2; ++i)
    {
        mesh->faces[i][0] = i % 2 ? i / 2 + sides : i / 2;
        mesh->faces[i][1] = (i / 2 + 1) % sides ? i / 2 + 1 : i / 2 + 1 - sides;
        mesh->faces[i][2] = (i + 1) % (sides * 2) ? (i + 1) / 2 + sides : (i + 1) / 2;
    }

    for (j = 0; i < numfaces; ++i, ++j)
    {
        mesh->faces[i][0] = i % 2 ? j / 2 : i / 2;
        mesh->faces[i][1] = (i / 2 + 1) % sides ? i / 2 + 1 : i / 2 + 1 - sides;
        mesh->faces[i][2] = i == numfaces - 1 ? 0 : (j + 1) / 2;
    }

    return TRUE;
}

static void test_torus(IDirect3DDevice9 *device, float innerradius, float outerradius, UINT sides, UINT rings)
{
    HRESULT hr;
    ID3DXMesh *torus;
    struct mesh mesh;
    char name[256];

    hr = D3DXCreateTorus(device, innerradius, outerradius, sides, rings, &torus, NULL);
    ok(hr == D3D_OK, "Got result %#lx, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create torus\n");
        return;
    }

    if (!compute_torus(&mesh, innerradius, outerradius, sides, rings))
    {
        skip("Couldn't create mesh\n");
        torus->lpVtbl->Release(torus);
        return;
    }

    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    sprintf(name, "torus (%g, %g, %u, %u)", innerradius, outerradius, sides, rings);
    compare_mesh(name, torus, &mesh);

    free_mesh(&mesh);

    torus->lpVtbl->Release(torus);
}

static void D3DXCreateTorusTest(void)
{
    HRESULT hr;
    IDirect3DDevice9* device;
    ID3DXMesh* torus = NULL;
    struct test_context *test_context;

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateTorus(NULL, 0.0f, 0.0f, 3, 3, &torus, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %#lx, expected %#lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTorus(device, -1.0f, 0.0f, 3, 3, &torus, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %#lx, expected %#lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTorus(device, 0.0f, -1.0f, 3, 3, &torus, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %#lx, expected %#lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTorus(device, 0.0f, 0.0f, 2, 3, &torus, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %#lx, expected %#lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTorus(device, 0.0f, 0.0f, 3, 2, &torus, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %#lx, expected %#lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTorus(device, 0.0f, 0.0f, 3, 3, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %#lx, expected %#lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    test_torus(device, 0.0f, 0.0f, 3, 3);
    test_torus(device, 1.0f, 1.0f, 3, 3);
    test_torus(device, 1.0f, 1.0f, 32, 64);
    test_torus(device, 0.0f, 1.0f, 5, 5);
    test_torus(device, 1.0f, 0.0f, 5, 5);
    test_torus(device, 5.0f, 0.2f, 8, 8);
    test_torus(device, 0.2f, 1.0f, 60, 3);
    test_torus(device, 0.2f, 1.0f, 8, 70);

    free_test_context(test_context);
}

struct dynamic_array
{
    int count, capacity;
    void *items;
};

enum pointtype {
    POINTTYPE_CURVE = 0,
    POINTTYPE_CORNER,
    POINTTYPE_CURVE_START,
    POINTTYPE_CURVE_END,
    POINTTYPE_CURVE_MIDDLE,
};

struct point2d
{
    D3DXVECTOR2 pos;
    enum pointtype corner;
};

/* is a dynamic_array */
struct outline
{
    int count, capacity;
    struct point2d *items;
};

/* is a dynamic_array */
struct outline_array
{
    int count, capacity;
    struct outline *items;
};

struct glyphinfo
{
    struct outline_array outlines;
    float offset_x;
};

static BOOL reserve(struct dynamic_array *array, int count, int itemsize)
{
    if (count > array->capacity) {
        void *new_buffer;
        int new_capacity;
        if (array->items && array->capacity) {
            new_capacity = max(array->capacity * 2, count);
            new_buffer = realloc(array->items, new_capacity * itemsize);
        } else {
            new_capacity = max(16, count);
            new_buffer = malloc(new_capacity * itemsize);
        }
        if (!new_buffer)
            return FALSE;
        array->items = new_buffer;
        array->capacity = new_capacity;
    }
    return TRUE;
}

static struct point2d *add_point(struct outline *array)
{
    struct point2d *item;

    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count++];
    ZeroMemory(item, sizeof(*item));
    return item;
}

static struct outline *add_outline(struct outline_array *array)
{
    struct outline *item;

    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count++];
    ZeroMemory(item, sizeof(*item));
    return item;
}

static inline D3DXVECTOR2 *convert_fixed_to_float(POINTFX *pt, int count, float emsquare)
{
    D3DXVECTOR2 *ret = (D3DXVECTOR2*)pt;
    while (count--) {
        D3DXVECTOR2 *pt_flt = (D3DXVECTOR2*)pt;
        pt_flt->x = (pt->x.value + pt->x.fract / (float)0x10000) / emsquare;
        pt_flt->y = (pt->y.value + pt->y.fract / (float)0x10000) / emsquare;
        pt++;
    }
    return ret;
}

static HRESULT add_bezier_points(struct outline *outline, const D3DXVECTOR2 *p1,
                                 const D3DXVECTOR2 *p2, const D3DXVECTOR2 *p3,
                                 float max_deviation)
{
    D3DXVECTOR2 split1 = {0, 0}, split2 = {0, 0}, middle, vec;
    float deviation;

    D3DXVec2Scale(&split1, D3DXVec2Add(&split1, p1, p2), 0.5f);
    D3DXVec2Scale(&split2, D3DXVec2Add(&split2, p2, p3), 0.5f);
    D3DXVec2Scale(&middle, D3DXVec2Add(&middle, &split1, &split2), 0.5f);

    deviation = D3DXVec2Length(D3DXVec2Subtract(&vec, &middle, p2));
    if (deviation < max_deviation) {
        struct point2d *pt = add_point(outline);
        if (!pt) return E_OUTOFMEMORY;
        pt->pos = *p2;
        pt->corner = POINTTYPE_CURVE;
        /* the end point is omitted because the end line merges into the next segment of
         * the split bezier curve, and the end of the split bezier curve is added outside
         * this recursive function. */
    } else {
        HRESULT hr = add_bezier_points(outline, p1, &split1, &middle, max_deviation);
        if (hr != S_OK) return hr;
        hr = add_bezier_points(outline, &middle, &split2, p3, max_deviation);
        if (hr != S_OK) return hr;
    }

    return S_OK;
}

static inline BOOL is_direction_similar(D3DXVECTOR2 *dir1, D3DXVECTOR2 *dir2, float cos_theta)
{
    /* dot product = cos(theta) */
    return D3DXVec2Dot(dir1, dir2) > cos_theta;
}

static inline D3DXVECTOR2 *unit_vec2(D3DXVECTOR2 *dir, const D3DXVECTOR2 *pt1, const D3DXVECTOR2 *pt2)
{
    return D3DXVec2Normalize(dir, D3DXVec2Subtract(dir, pt2, pt1));
}

static BOOL attempt_line_merge(struct outline *outline,
                               int pt_index,
                               const D3DXVECTOR2 *nextpt,
                               BOOL to_curve)
{
    D3DXVECTOR2 curdir, lastdir;
    struct point2d *prevpt, *pt;
    BOOL ret = FALSE;
    const float cos_half = cos(D3DXToRadian(0.5f));

    pt = &outline->items[pt_index];
    pt_index = (pt_index - 1 + outline->count) % outline->count;
    prevpt = &outline->items[pt_index];

    if (to_curve)
        pt->corner = pt->corner != POINTTYPE_CORNER ? POINTTYPE_CURVE_MIDDLE : POINTTYPE_CURVE_START;

    if (outline->count < 2)
        return FALSE;

    /* remove last point if the next line continues the last line */
    unit_vec2(&lastdir, &prevpt->pos, &pt->pos);
    unit_vec2(&curdir, &pt->pos, nextpt);
    if (is_direction_similar(&lastdir, &curdir, cos_half))
    {
        outline->count--;
        if (pt->corner == POINTTYPE_CURVE_END)
            prevpt->corner = pt->corner;
        if (prevpt->corner == POINTTYPE_CURVE_END && to_curve)
            prevpt->corner = POINTTYPE_CURVE_MIDDLE;
        pt = prevpt;

        ret = TRUE;
        if (outline->count < 2)
            return ret;

        pt_index = (pt_index - 1 + outline->count) % outline->count;
        prevpt = &outline->items[pt_index];
        unit_vec2(&lastdir, &prevpt->pos, &pt->pos);
        unit_vec2(&curdir, &pt->pos, nextpt);
    }
    return ret;
}

static HRESULT create_outline(struct glyphinfo *glyph, void *raw_outline, int datasize,
                              float max_deviation, float emsquare)
{
    const float cos_45 = cos(D3DXToRadian(45.0f));
    const float cos_90 = cos(D3DXToRadian(90.0f));
    TTPOLYGONHEADER *header = (TTPOLYGONHEADER *)raw_outline;

    while ((char *)header < (char *)raw_outline + datasize)
    {
        TTPOLYCURVE *curve = (TTPOLYCURVE *)(header + 1);
        struct point2d *lastpt, *pt;
        D3DXVECTOR2 lastdir;
        D3DXVECTOR2 *pt_flt;
        int j;
        struct outline *outline = add_outline(&glyph->outlines);

        if (!outline)
            return E_OUTOFMEMORY;

        pt = add_point(outline);
        if (!pt)
            return E_OUTOFMEMORY;
        pt_flt = convert_fixed_to_float(&header->pfxStart, 1, emsquare);
        pt->pos = *pt_flt;
        pt->corner = POINTTYPE_CORNER;

        if (header->dwType != TT_POLYGON_TYPE)
            trace("Unknown header type %ld\n", header->dwType);

        while ((char *)curve < (char *)header + header->cb)
        {
            D3DXVECTOR2 bezier_start = outline->items[outline->count - 1].pos;
            BOOL to_curve = curve->wType != TT_PRIM_LINE && curve->cpfx > 1;

            if (!curve->cpfx) {
                curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
                continue;
            }

            pt_flt = convert_fixed_to_float(curve->apfx, curve->cpfx, emsquare);

            attempt_line_merge(outline, outline->count - 1, &pt_flt[0], to_curve);

            if (to_curve)
            {
                HRESULT hr;
                int count = curve->cpfx;
                j = 0;

                while (count > 2)
                {
                    D3DXVECTOR2 bezier_end;

                    D3DXVec2Scale(&bezier_end, D3DXVec2Add(&bezier_end, &pt_flt[j], &pt_flt[j+1]), 0.5f);
                    hr = add_bezier_points(outline, &bezier_start, &pt_flt[j], &bezier_end, max_deviation);
                    if (hr != S_OK)
                        return hr;
                    bezier_start = bezier_end;
                    count--;
                    j++;
                }
                hr = add_bezier_points(outline, &bezier_start, &pt_flt[j], &pt_flt[j+1], max_deviation);
                if (hr != S_OK)
                    return hr;

                pt = add_point(outline);
                if (!pt)
                    return E_OUTOFMEMORY;
                j++;
                pt->pos = pt_flt[j];
                pt->corner = POINTTYPE_CURVE_END;
            } else {
                for (j = 0; j < curve->cpfx; j++)
                {
                    pt = add_point(outline);
                    if (!pt)
                        return E_OUTOFMEMORY;
                    pt->pos = pt_flt[j];
                    pt->corner = POINTTYPE_CORNER;
                }
            }

            curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
        }

        /* remove last point if the next line continues the last line */
        if (outline->count >= 3) {
            BOOL to_curve;

            lastpt = &outline->items[outline->count - 1];
            pt = &outline->items[0];
            if (pt->pos.x == lastpt->pos.x && pt->pos.y == lastpt->pos.y) {
                if (lastpt->corner == POINTTYPE_CURVE_END)
                {
                    if (pt->corner == POINTTYPE_CURVE_START)
                        pt->corner = POINTTYPE_CURVE_MIDDLE;
                    else
                        pt->corner = POINTTYPE_CURVE_END;
                }
                outline->count--;
            } else {
                /* outline closed with a line from end to start point */
                attempt_line_merge(outline, outline->count - 1, &pt->pos, FALSE);
            }
            lastpt = &outline->items[0];
            to_curve = lastpt->corner != POINTTYPE_CORNER && lastpt->corner != POINTTYPE_CURVE_END;
            if (lastpt->corner == POINTTYPE_CURVE_START)
                lastpt->corner = POINTTYPE_CORNER;
            pt = &outline->items[1];
            if (attempt_line_merge(outline, 0, &pt->pos, to_curve))
                *lastpt = outline->items[outline->count];
        }

        lastpt = &outline->items[outline->count - 1];
        pt = &outline->items[0];
        unit_vec2(&lastdir, &lastpt->pos, &pt->pos);
        for (j = 0; j < outline->count; j++)
        {
            D3DXVECTOR2 curdir;

            lastpt = pt;
            pt = &outline->items[(j + 1) % outline->count];
            unit_vec2(&curdir, &lastpt->pos, &pt->pos);

            switch (lastpt->corner)
            {
                case POINTTYPE_CURVE_START:
                case POINTTYPE_CURVE_END:
                    if (!is_direction_similar(&lastdir, &curdir, cos_45))
                        lastpt->corner = POINTTYPE_CORNER;
                    break;
                case POINTTYPE_CURVE_MIDDLE:
                    if (!is_direction_similar(&lastdir, &curdir, cos_90))
                        lastpt->corner = POINTTYPE_CORNER;
                    else
                        lastpt->corner = POINTTYPE_CURVE;
                    break;
                default:
                    break;
            }
            lastdir = curdir;
        }

        header = (TTPOLYGONHEADER *)((char *)header + header->cb);
    }
    return S_OK;
}

static void free_outline(struct outline *outline)
{
    free(outline->items);
}

static void free_glyphinfo(struct glyphinfo *glyph)
{
    unsigned int i;

    for (i = 0; i < glyph->outlines.count; ++i)
        free_outline(&glyph->outlines.items[i]);
    free(glyph->outlines.items);
}

static void compute_text_mesh(struct mesh *mesh, const char *text,
        float deviation, float extrusion, float otmEMSquare, const struct glyphinfo *glyphs)
{
    DWORD nb_vertices, nb_faces;
    DWORD nb_corners, nb_outline_points;
    int textlen = 0;
    int i;
    struct vertex *vertex_ptr;
    face *face_ptr;

    textlen = strlen(text);

    /* corner points need an extra vertex for the different side faces normals */
    nb_corners = 0;
    nb_outline_points = 0;
    for (i = 0; i < textlen; i++)
    {
        int j;
        for (j = 0; j < glyphs[i].outlines.count; j++)
        {
            int k;
            struct outline *outline = &glyphs[i].outlines.items[j];
            nb_outline_points += outline->count;
            nb_corners++; /* first outline point always repeated as a corner */
            for (k = 1; k < outline->count; k++)
                if (outline->items[k].corner)
                    nb_corners++;
        }
    }

    nb_vertices = (nb_outline_points + nb_corners) * 2 + textlen;
    nb_faces = nb_outline_points * 2;

    ok(new_mesh(mesh, nb_vertices, nb_faces), "Failed to create reference text mesh.\n");

    /* convert 2D vertices and faces into 3D mesh */
    vertex_ptr = mesh->vertices;
    face_ptr = mesh->faces;
    for (i = 0; i < textlen; i++)
    {
        int j;

        /* side vertices and faces */
        for (j = 0; j < glyphs[i].outlines.count; j++)
        {
            struct vertex *outline_vertices = vertex_ptr;
            struct outline *outline = &glyphs[i].outlines.items[j];
            int k;
            struct point2d *prevpt = &outline->items[outline->count - 1];
            struct point2d *pt = &outline->items[0];

            for (k = 1; k <= outline->count; k++)
            {
                struct vertex vtx;
                struct point2d *nextpt = &outline->items[k % outline->count];
                WORD vtx_idx = vertex_ptr - mesh->vertices;
                D3DXVECTOR2 vec;

                if (pt->corner == POINTTYPE_CURVE_START)
                    D3DXVec2Subtract(&vec, &pt->pos, &prevpt->pos);
                else if (pt->corner)
                    D3DXVec2Subtract(&vec, &nextpt->pos, &pt->pos);
                else
                    D3DXVec2Subtract(&vec, &nextpt->pos, &prevpt->pos);
                D3DXVec2Normalize(&vec, &vec);
                vtx.normal.x = -vec.y;
                vtx.normal.y = vec.x;
                vtx.normal.z = 0;

                vtx.position.x = pt->pos.x + glyphs[i].offset_x;
                vtx.position.y = pt->pos.y;
                vtx.position.z = 0;
                *vertex_ptr++ = vtx;

                vtx.position.z = -extrusion;
                *vertex_ptr++ = vtx;

                vtx.position.x = nextpt->pos.x + glyphs[i].offset_x;
                vtx.position.y = nextpt->pos.y;
                if (pt->corner && nextpt->corner && nextpt->corner != POINTTYPE_CURVE_END) {
                    vtx.position.z = -extrusion;
                    *vertex_ptr++ = vtx;
                    vtx.position.z = 0;
                    *vertex_ptr++ = vtx;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 2;
                    (*face_ptr)[2] = vtx_idx + 1;
                    face_ptr++;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 3;
                    (*face_ptr)[2] = vtx_idx + 2;
                    face_ptr++;
                } else {
                    if (nextpt->corner) {
                        if (nextpt->corner == POINTTYPE_CURVE_END) {
                            struct point2d *nextpt2 = &outline->items[(k + 1) % outline->count];
                            D3DXVec2Subtract(&vec, &nextpt2->pos, &nextpt->pos);
                        } else {
                            D3DXVec2Subtract(&vec, &nextpt->pos, &pt->pos);
                        }
                        D3DXVec2Normalize(&vec, &vec);
                        vtx.normal.x = -vec.y;
                        vtx.normal.y = vec.x;

                        vtx.position.z = 0;
                        *vertex_ptr++ = vtx;
                        vtx.position.z = -extrusion;
                        *vertex_ptr++ = vtx;
                    }

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 3;
                    (*face_ptr)[2] = vtx_idx + 1;
                    face_ptr++;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 2;
                    (*face_ptr)[2] = vtx_idx + 3;
                    face_ptr++;
                }

                prevpt = pt;
                pt = nextpt;
            }
            if (!pt->corner) {
                *vertex_ptr++ = *outline_vertices++;
                *vertex_ptr++ = *outline_vertices++;
            }
        }

        /* FIXME: compute expected faces */
        /* Add placeholder to separate glyph outlines */
        vertex_ptr->position.x = 0;
        vertex_ptr->position.y = 0;
        vertex_ptr->position.z = 0;
        vertex_ptr->normal.x = 0;
        vertex_ptr->normal.y = 0;
        vertex_ptr->normal.z = 1;
        vertex_ptr++;
    }
}

static void compare_text_outline_mesh(const char *name, ID3DXMesh *d3dxmesh, struct mesh *mesh,
        size_t textlen, float extrusion, const struct glyphinfo *glyphs)
{
    HRESULT hr;
    DWORD number_of_vertices, number_of_faces;
    IDirect3DVertexBuffer9 *vertex_buffer = NULL;
    IDirect3DIndexBuffer9 *index_buffer = NULL;
    D3DVERTEXBUFFER_DESC vertex_buffer_description;
    D3DINDEXBUFFER_DESC index_buffer_description;
    struct vertex *vertices = NULL;
    face *faces = NULL;
    int expected, i;
    int vtx_idx1, face_idx1, vtx_idx2, face_idx2;

    number_of_vertices = d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh);
    number_of_faces = d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh);

    hr = d3dxmesh->lpVtbl->GetVertexBuffer(d3dxmesh, &vertex_buffer);
    ok(hr == D3D_OK, "Test %s, unexpected hr %#lx.\n", name, hr);
    hr = IDirect3DVertexBuffer9_GetDesc(vertex_buffer, &vertex_buffer_description);
    ok(hr == D3D_OK, "Test %s, unexpected hr %#lx.\n", name, hr);
    ok(vertex_buffer_description.Format == D3DFMT_VERTEXDATA, "Test %s, unexpected format %u.\n",
            name, vertex_buffer_description.Format);
    ok(vertex_buffer_description.Type == D3DRTYPE_VERTEXBUFFER, "Test %s, unexpected resource type %u.\n",
            name, vertex_buffer_description.Type);
    ok(!vertex_buffer_description.Usage, "Test %s, unexpected usage %#lx.\n", name, vertex_buffer_description.Usage);
    ok(vertex_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, unexpected pool %u.\n",
            name, vertex_buffer_description.Pool);
    ok(vertex_buffer_description.FVF == mesh->fvf, "Test %s, unexpected FVF %#lx (expected %#lx).\n",
            name, vertex_buffer_description.FVF, mesh->fvf);
    if (!mesh->fvf)
        expected = number_of_vertices * mesh->vertex_size;
    else
        expected = number_of_vertices * D3DXGetFVFVertexSize(mesh->fvf);
    ok(vertex_buffer_description.Size == expected, "Test %s, unexpected size %u (expected %u).\n",
            name, vertex_buffer_description.Size, expected);

    hr = d3dxmesh->lpVtbl->GetIndexBuffer(d3dxmesh, &index_buffer);
    ok(hr == D3D_OK, "Test %s, unexpected hr %#lx.\n", name, hr);
    hr = IDirect3DIndexBuffer9_GetDesc(index_buffer, &index_buffer_description);
    ok(hr == D3D_OK, "Test %s, unexpected hr %#lx.\n", name, hr);
    ok(index_buffer_description.Format == D3DFMT_INDEX16, "Test %s, unexpected format %u.\n",
            name, index_buffer_description.Format);
    ok(index_buffer_description.Type == D3DRTYPE_INDEXBUFFER, "Test %s, unexpected resource type %u.\n",
            name, index_buffer_description.Type);
    ok(!index_buffer_description.Usage, "Test %s, unexpected usage %#lx.\n",
            name, index_buffer_description.Usage);
    ok(index_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, unexpected pool %u.\n",
            name, index_buffer_description.Pool);
    expected = number_of_faces * sizeof(WORD) * 3;
    ok(index_buffer_description.Size == expected, "Test %s, unexpected size %u.\n",
            name, index_buffer_description.Size);

    hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, number_of_vertices * sizeof(D3DXVECTOR3) * 2,
            (void **)&vertices, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "Test %s, unexpected hr %#lx.\n", name, hr);
    hr = IDirect3DIndexBuffer9_Lock(index_buffer, 0, number_of_faces * sizeof(WORD) * 3,
            (void **)&faces, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "Test %s, unexpected hr %#lx.\n", name, hr);
    face_idx1 = 0;
    vtx_idx2 = 0;
    face_idx2 = 0;
    vtx_idx1 = 0;
    for (i = 0; i < textlen; i++)
    {
        int nb_outline_vertices1, nb_outline_faces1;
        int nb_outline_vertices2, nb_outline_faces2;
        int nb_back_vertices, nb_back_faces;
        int first_vtx1, first_vtx2;
        int first_face1, first_face2;
        int j;

        first_vtx1 = vtx_idx1;
        first_vtx2 = vtx_idx2;
        /* Glyphs without outlines do not generate any vertices. */
        if (glyphs[i].outlines.count > 0)
        {
            for (; vtx_idx1 < number_of_vertices; vtx_idx1++)
            {
                if (vertices[vtx_idx1].normal.z != 0)
                    break;
            }

            for (; vtx_idx2 < mesh->number_of_vertices; vtx_idx2++)
            {
                if (mesh->vertices[vtx_idx2].normal.z != 0)
                    break;
            }
        }
        nb_outline_vertices1 = vtx_idx1 - first_vtx1;
        nb_outline_vertices2 = vtx_idx2 - first_vtx2;
        ok(nb_outline_vertices1 == nb_outline_vertices2,
           "Test %s, glyph %d, outline vertex count result %d, expected %d\n", name, i,
           nb_outline_vertices1, nb_outline_vertices2);

        for (j = 0; j < min(nb_outline_vertices1, nb_outline_vertices2); j++)
        {
            vtx_idx1 = first_vtx1 + j;
            vtx_idx2 = first_vtx2 + j;
            ok(compare_vec3(vertices[vtx_idx1].position, mesh->vertices[vtx_idx2].position),
               "Test %s, glyph %d, vertex position %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].position.x, vertices[vtx_idx1].position.y, vertices[vtx_idx1].position.z,
               mesh->vertices[vtx_idx2].position.x, mesh->vertices[vtx_idx2].position.y, mesh->vertices[vtx_idx2].position.z);
            ok(compare_vec3(vertices[vtx_idx1].normal, mesh->vertices[first_vtx2 + j].normal),
               "Test %s, glyph %d, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].normal.x, vertices[vtx_idx1].normal.y, vertices[vtx_idx1].normal.z,
               mesh->vertices[vtx_idx2].normal.x, mesh->vertices[vtx_idx2].normal.y, mesh->vertices[vtx_idx2].normal.z);
        }
        vtx_idx1 = first_vtx1 + nb_outline_vertices1;
        vtx_idx2 = first_vtx2 + nb_outline_vertices2;

        first_face1 = face_idx1;
        first_face2 = face_idx2;
        for (; face_idx1 < number_of_faces; face_idx1++)
        {
            if (faces[face_idx1][0] >= vtx_idx1 ||
                faces[face_idx1][1] >= vtx_idx1 ||
                faces[face_idx1][2] >= vtx_idx1)
                break;
        }
        for (; face_idx2 < mesh->number_of_faces; face_idx2++)
        {
            if (mesh->faces[face_idx2][0] >= vtx_idx2 ||
                mesh->faces[face_idx2][1] >= vtx_idx2 ||
                mesh->faces[face_idx2][2] >= vtx_idx2)
                break;
        }
        nb_outline_faces1 = face_idx1 - first_face1;
        nb_outline_faces2 = face_idx2 - first_face2;
        ok(nb_outline_faces1 == nb_outline_faces2,
           "Test %s, glyph %d, outline face count result %d, expected %d\n", name, i,
           nb_outline_faces1, nb_outline_faces2);

        for (j = 0; j < min(nb_outline_faces1, nb_outline_faces2); j++)
        {
            face_idx1 = first_face1 + j;
            face_idx2 = first_face2 + j;
            ok(faces[face_idx1][0] - first_vtx1 == mesh->faces[face_idx2][0] - first_vtx2 &&
               faces[face_idx1][1] - first_vtx1 == mesh->faces[face_idx2][1] - first_vtx2 &&
               faces[face_idx1][2] - first_vtx1 == mesh->faces[face_idx2][2] - first_vtx2,
               "Test %s, glyph %d, face %d, result (%d, %d, %d), expected (%d, %d, %d)\n", name, i, face_idx1,
               faces[face_idx1][0], faces[face_idx1][1], faces[face_idx1][2],
               mesh->faces[face_idx2][0] - first_vtx2 + first_vtx1,
               mesh->faces[face_idx2][1] - first_vtx2 + first_vtx1,
               mesh->faces[face_idx2][2] - first_vtx2 + first_vtx1);
        }
        face_idx1 = first_face1 + nb_outline_faces1;
        face_idx2 = first_face2 + nb_outline_faces2;

        /* partial test on back vertices and faces  */
        first_vtx1 = vtx_idx1;
        for (; vtx_idx1 < number_of_vertices; vtx_idx1++) {
            struct vertex vtx;

            if (vertices[vtx_idx1].normal.z != 1.0f)
                break;

            vtx.position.z = 0.0f;
            vtx.normal.x = 0.0f;
            vtx.normal.y = 0.0f;
            vtx.normal.z = 1.0f;
            ok(compare(vertices[vtx_idx1].position.z, vtx.position.z),
               "Test %s, glyph %d, vertex position.z %d, result %g, expected %g\n", name, i, vtx_idx1,
               vertices[vtx_idx1].position.z, vtx.position.z);
            ok(compare_vec3(vertices[vtx_idx1].normal, vtx.normal),
               "Test %s, glyph %d, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].normal.x, vertices[vtx_idx1].normal.y, vertices[vtx_idx1].normal.z,
               vtx.normal.x, vtx.normal.y, vtx.normal.z);
        }
        nb_back_vertices = vtx_idx1 - first_vtx1;
        first_face1 = face_idx1;
        for (; face_idx1 < number_of_faces; face_idx1++)
        {
            const D3DXVECTOR3 *vtx1, *vtx2, *vtx3;
            D3DXVECTOR3 normal;
            D3DXVECTOR3 v1 = {0, 0, 0};
            D3DXVECTOR3 v2 = {0, 0, 0};
            D3DXVECTOR3 forward = {0.0f, 0.0f, 1.0f};

            if (faces[face_idx1][0] >= vtx_idx1 ||
                faces[face_idx1][1] >= vtx_idx1 ||
                faces[face_idx1][2] >= vtx_idx1)
                break;

            vtx1 = &vertices[faces[face_idx1][0]].position;
            vtx2 = &vertices[faces[face_idx1][1]].position;
            vtx3 = &vertices[faces[face_idx1][2]].position;

            D3DXVec3Subtract(&v1, vtx2, vtx1);
            D3DXVec3Subtract(&v2, vtx3, vtx2);
            D3DXVec3Cross(&normal, &v1, &v2);
            D3DXVec3Normalize(&normal, &normal);
            ok(!D3DXVec3Length(&normal) || compare_vec3(normal, forward),
               "Test %s, glyph %d, face %d normal, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, face_idx1,
               normal.x, normal.y, normal.z, forward.x, forward.y, forward.z);
        }
        nb_back_faces = face_idx1 - first_face1;

        /* compare front and back faces & vertices */
        if (extrusion == 0.0f) {
            /* Oddly there are only back faces in this case */
            nb_back_vertices /= 2;
            nb_back_faces /= 2;
            face_idx1 -= nb_back_faces;
            vtx_idx1 -= nb_back_vertices;
        }
        for (j = 0; j < nb_back_vertices; j++)
        {
            struct vertex vtx = vertices[first_vtx1];
            vtx.position.z = -extrusion;
            vtx.normal.x = 0.0f;
            vtx.normal.y = 0.0f;
            vtx.normal.z = extrusion == 0.0f ? 1.0f : -1.0f;
            ok(compare_vec3(vertices[vtx_idx1].position, vtx.position),
               "Test %s, glyph %d, vertex position %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].position.x, vertices[vtx_idx1].position.y, vertices[vtx_idx1].position.z,
               vtx.position.x, vtx.position.y, vtx.position.z);
            ok(compare_vec3(vertices[vtx_idx1].normal, vtx.normal),
               "Test %s, glyph %d, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].normal.x, vertices[vtx_idx1].normal.y, vertices[vtx_idx1].normal.z,
               vtx.normal.x, vtx.normal.y, vtx.normal.z);
            vtx_idx1++;
            first_vtx1++;
        }
        for (j = 0; j < nb_back_faces; j++)
        {
            int f1, f2;
            if (extrusion == 0.0f) {
                f1 = 1;
                f2 = 2;
            } else {
                f1 = 2;
                f2 = 1;
            }
            ok(faces[face_idx1][0] == faces[first_face1][0] + nb_back_vertices &&
               faces[face_idx1][1] == faces[first_face1][f1] + nb_back_vertices &&
               faces[face_idx1][2] == faces[first_face1][f2] + nb_back_vertices,
               "Test %s, glyph %d, face %d, result (%d, %d, %d), expected (%d, %d, %d)\n", name, i, face_idx1,
               faces[face_idx1][0], faces[face_idx1][1], faces[face_idx1][2],
               faces[first_face1][0] - nb_back_faces,
               faces[first_face1][f1] - nb_back_faces,
               faces[first_face1][f2] - nb_back_faces);
            first_face1++;
            face_idx1++;
        }

        /* skip to the outline for the next glyph */
        for (; vtx_idx2 < mesh->number_of_vertices; vtx_idx2++) {
            if (mesh->vertices[vtx_idx2].normal.z == 0)
                break;
        }
        for (; face_idx2 < mesh->number_of_faces; face_idx2++)
        {
            if (mesh->faces[face_idx2][0] >= vtx_idx2 ||
                mesh->faces[face_idx2][1] >= vtx_idx2 ||
                mesh->faces[face_idx2][2] >= vtx_idx2) break;
        }
    }

    IDirect3DIndexBuffer9_Unlock(index_buffer);
    IDirect3DVertexBuffer9_Unlock(vertex_buffer);
    IDirect3DIndexBuffer9_Release(index_buffer);
    IDirect3DVertexBuffer9_Release(vertex_buffer);
}

static void test_createtext(IDirect3DDevice9 *device, HDC hdc, const char *text, float deviation, float extrusion)
{
    static const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    HRESULT hr;
    ID3DXMesh *d3dxmesh = NULL;
    struct mesh mesh = {0};
    char name[256];
    OUTLINETEXTMETRICA otm;
    GLYPHMETRICS gm;
    struct glyphinfo *glyphs;
    GLYPHMETRICSFLOAT *glyphmetrics_float = malloc(sizeof(GLYPHMETRICSFLOAT) * strlen(text));
    int i;
    LOGFONTA lf;
    float offset_x;
    size_t textlen;
    HFONT font = NULL, oldfont = NULL;
    char *raw_outline;

    sprintf(name, "text ('%s', %f, %f)", text, deviation, extrusion);

    hr = D3DXCreateTextA(device, hdc, text, deviation, extrusion, &d3dxmesh, NULL, glyphmetrics_float);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

    /* must select a modified font having lfHeight = otm.otmEMSquare before
     * calling GetGlyphOutline to get the expected values */
    ok(GetObjectA(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf), "Failed to get current DC font.\n");
    ok(GetOutlineTextMetricsA(hdc, sizeof(otm), &otm), "Failed to get DC font outline.\n");
    lf.lfHeight = otm.otmEMSquare;
    lf.lfWidth = 0;
    ok(!!(font = CreateFontIndirectA(&lf)), "Failed to create font.\n");

    textlen = strlen(text);
    glyphs = calloc(textlen, sizeof(*glyphs));
    oldfont = SelectObject(hdc, font);

    for (i = 0; i < textlen; i++)
    {
        GetGlyphOutlineA(hdc, text[i], GGO_NATIVE, &gm, 0, NULL, &identity);
        compare_float(glyphmetrics_float[i].gmfBlackBoxX, gm.gmBlackBoxX / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfBlackBoxY, gm.gmBlackBoxY / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfptGlyphOrigin.x, gm.gmptGlyphOrigin.x / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfptGlyphOrigin.y, gm.gmptGlyphOrigin.y / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfCellIncX, gm.gmCellIncX / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfCellIncY, gm.gmCellIncY / (float)otm.otmEMSquare);
    }

    if (deviation == 0.0f)
        deviation = 1.0f / otm.otmEMSquare;

    offset_x = 0.0f;
    for (i = 0; i < textlen; i++)
    {
        DWORD datasize;

        glyphs[i].offset_x = offset_x;

        datasize = GetGlyphOutlineA(hdc, text[i], GGO_NATIVE, &gm, 0, NULL, &identity);
        ok(datasize != GDI_ERROR, "Failed to retrieve GDI glyph outline size.\n");
        raw_outline = malloc(datasize);
        datasize = GetGlyphOutlineA(hdc, text[i], GGO_NATIVE, &gm, datasize, raw_outline, &identity);
        ok(datasize != GDI_ERROR, "Failed to retrieve GDI glyph outline.\n");
        create_outline(&glyphs[i], raw_outline, datasize, deviation, otm.otmEMSquare);
        free(raw_outline);

        offset_x += gm.gmCellIncX / (float)otm.otmEMSquare;
    }

    SelectObject(hdc, oldfont);

    compute_text_mesh(&mesh, text, deviation, extrusion, otm.otmEMSquare, glyphs);
    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    compare_text_outline_mesh(name, d3dxmesh, &mesh, textlen, extrusion, glyphs);

    free_mesh(&mesh);
    d3dxmesh->lpVtbl->Release(d3dxmesh);
    DeleteObject(font);
    free(glyphmetrics_float);

    for (i = 0; i < textlen; i++)
        free_glyphinfo(&glyphs[i]);
    free(glyphs);
}

static void D3DXCreateTextTest(void)
{
    HRESULT hr;
    HDC hdc;
    IDirect3DDevice9* device;
    ID3DXMesh* d3dxmesh = NULL;
    HFONT hFont;
    OUTLINETEXTMETRICA otm;
    int number_of_vertices;
    int number_of_faces;
    struct test_context *test_context;

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hdc = CreateCompatibleDC(NULL);

    hFont = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SelectObject(hdc, hFont);
    GetOutlineTextMetricsA(hdc, sizeof(otm), &otm);

    hr = D3DXCreateTextA(device, hdc, "wine", 0.001f, 0.4f, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    /* D3DXCreateTextA page faults from passing NULL text */

    hr = D3DXCreateTextW(device, hdc, NULL, 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextA(device, hdc, "", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextA(device, hdc, " ", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextA(NULL, hdc, "wine", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextA(device, NULL, "wine", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextA(device, hdc, "wine", -FLT_MIN, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextA(device, hdc, "wine", 0.001f, -FLT_MIN, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    /* deviation = 0.0f treated as if deviation = 1.0f / otm.otmEMSquare */
    hr = D3DXCreateTextA(device, hdc, "wine", 1.0f / otm.otmEMSquare, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    number_of_vertices = d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh);
    number_of_faces = d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh);
    d3dxmesh->lpVtbl->Release(d3dxmesh);

    hr = D3DXCreateTextA(device, hdc, "wine", 0.0f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    ok(number_of_vertices == d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh),
       "Got %ld vertices, expected %d\n",
       d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh), number_of_vertices);
    ok(number_of_faces == d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh),
       "Got %ld faces, expected %d\n",
       d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh), number_of_faces);
    d3dxmesh->lpVtbl->Release(d3dxmesh);

if (0)
{
    /* too much detail requested, so will appear to hang */
    trace("Waiting for D3DXCreateText to finish with deviation = FLT_MIN ...\n");
    hr = D3DXCreateTextA(device, hdc, "wine", FLT_MIN, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    if (SUCCEEDED(hr) && d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);
    trace("D3DXCreateText finish with deviation = FLT_MIN\n");
}

    hr = D3DXCreateTextA(device, hdc, "wine", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    if (SUCCEEDED(hr) && d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);

    test_createtext(device, hdc, "wine", FLT_MAX, 0.4f);
    test_createtext(device, hdc, "wine", 0.001f, FLT_MIN);
    test_createtext(device, hdc, "wine", 0.001f, 0.0f);
    test_createtext(device, hdc, "wine", 0.001f, FLT_MAX);
    test_createtext(device, hdc, "wine", 0.0f, 1.0f);
    test_createtext(device, hdc, " wine", 1.0f, 0.0f);
    test_createtext(device, hdc, "wine ", 1.0f, 0.0f);
    test_createtext(device, hdc, "wi ne", 1.0f, 0.0f);

    DeleteDC(hdc);
    DeleteObject(hFont);

    free_test_context(test_context);
}

static void test_get_decl_length(void)
{
    static const D3DVERTEXELEMENT9 declaration1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 0, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 0, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 0, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {8, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {9, 0, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {10, 0, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {11, 0, D3DDECLTYPE_UDEC3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {12, 0, D3DDECLTYPE_DEC3N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {13, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {14, 0, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END(),
    };
    static const D3DVERTEXELEMENT9 declaration2[] =
    {
        {0, 8, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 8, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 8, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 8, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 8, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 8, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 8, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 8, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 8, D3DDECLTYPE_UDEC3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 8, D3DDECLTYPE_DEC3N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 8, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 8, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 8, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END(),
    };
    UINT size;

    size = D3DXGetDeclLength(declaration1);
    ok(size == 15, "Got size %u, expected 15.\n", size);

    size = D3DXGetDeclLength(declaration2);
    ok(size == 16, "Got size %u, expected 16.\n", size);
}

static void test_get_decl_vertex_size(void)
{
    static const D3DVERTEXELEMENT9 declaration1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 0, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 0, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 0, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {8, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {9, 0, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {10, 0, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {11, 0, D3DDECLTYPE_UDEC3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {12, 0, D3DDECLTYPE_DEC3N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {13, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {14, 0, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END(),
    };
    static const D3DVERTEXELEMENT9 declaration2[] =
    {
        {0, 8, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 8, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 8, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 8, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 8, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 8, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 8, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 8, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 8, D3DDECLTYPE_UDEC3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 8, D3DDECLTYPE_DEC3N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 8, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 8, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 8, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END(),
    };
    static const UINT sizes1[] =
    {
        4,  8,  12, 16,
        4,  4,  4,  8,
        4,  4,  8,  4,
        4,  4,  8,  0,
    };
    static const UINT sizes2[] =
    {
        12, 16, 20, 24,
        12, 12, 16, 16,
    };
    unsigned int i;
    UINT size;

    size = D3DXGetDeclVertexSize(NULL, 0);
    ok(size == 0, "Got size %#x, expected 0.\n", size);

    for (i = 0; i < 16; ++i)
    {
        size = D3DXGetDeclVertexSize(declaration1, i);
        ok(size == sizes1[i], "Got size %u for stream %u, expected %u.\n", size, i, sizes1[i]);
    }

    for (i = 0; i < 8; ++i)
    {
        size = D3DXGetDeclVertexSize(declaration2, i);
        ok(size == sizes2[i], "Got size %u for stream %u, expected %u.\n", size, i, sizes2[i]);
    }
}

static void D3DXGenerateAdjacencyTest(void)
{
    HRESULT hr;
    IDirect3DDevice9 *device;
    ID3DXMesh *d3dxmesh = NULL;
    D3DXVECTOR3 *vertices = NULL;
    WORD *indices = NULL;
    int i;
    struct {
        DWORD num_vertices;
        D3DXVECTOR3 vertices[6];
        DWORD num_faces;
        WORD indices[3 * 3];
        FLOAT epsilon;
        DWORD adjacency[3 * 3];
    } test_data[] = {
        { /* for epsilon < 0, indices must match for faces to be adjacent */
            4, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  0, 2, 3},
            -1.0,
            {-1, -1, 1,  0, -1, -1},
        },
        {
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  3, 4, 5},
            -1.0,
            {-1, -1, -1,  -1, -1, -1},
        },
        { /* for epsilon == 0, indices or vertices must match for faces to be adjacent */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  3, 4, 5},
            0.0,
            {-1, -1, 1,  0, -1, -1},
        },
        { /* for epsilon > 0, vertices must be less than (but NOT equal to) epsilon distance away */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.25}, {1.0, 1.0, 0.25}, {0.0, 1.0, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.25,
            {-1, -1, -1,  -1, -1, -1},
        },
        { /* for epsilon > 0, vertices must be less than (but NOT equal to) epsilon distance away */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.25}, {1.0, 1.0, 0.25}, {0.0, 1.0, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.250001,
            {-1, -1, 1,  0, -1, -1},
        },
        { /* length between vertices are compared to epsilon, not the individual dimension deltas */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.25, 0.25}, {1.0, 1.25, 0.25}, {0.0, 1.25, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.353, /* < sqrt(0.25*0.25 + 0.25*0.25) */
            {-1, -1, -1,  -1, -1, -1},
        },
        {
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.25, 0.25}, {1.0, 1.25, 0.25}, {0.0, 1.25, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.354, /* > sqrt(0.25*0.25 + 0.25*0.25) */
            {-1, -1, 1,  0, -1, -1},
        },
        { /* adjacent faces must have opposite winding orders at the shared edge */
            4, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  0, 3, 2},
            0.0,
            {-1, -1, -1,  -1, -1, -1},
        },
    };
    struct test_context *test_context;

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        DWORD adjacency[ARRAY_SIZE(test_data[0].adjacency)];
        int j;

        if (d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);
        d3dxmesh = NULL;

        hr = D3DXCreateMeshFVF(test_data[i].num_faces, test_data[i].num_vertices, 0, D3DFVF_XYZ, device, &d3dxmesh);
        ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);

        hr = d3dxmesh->lpVtbl->LockVertexBuffer(d3dxmesh, D3DLOCK_DISCARD, (void**)&vertices);
        ok(hr == D3D_OK, "test %d: Got result %lx, expected %lx (D3D_OK)\n", i, hr, D3D_OK);
        if (FAILED(hr)) continue;
        CopyMemory(vertices, test_data[i].vertices, test_data[i].num_vertices * sizeof(test_data[0].vertices[0]));
        d3dxmesh->lpVtbl->UnlockVertexBuffer(d3dxmesh);

        hr = d3dxmesh->lpVtbl->LockIndexBuffer(d3dxmesh, D3DLOCK_DISCARD, (void**)&indices);
        ok(hr == D3D_OK, "test %d: Got result %lx, expected %lx (D3D_OK)\n", i, hr, D3D_OK);
        if (FAILED(hr)) continue;
        CopyMemory(indices, test_data[i].indices, test_data[i].num_faces * 3 * sizeof(test_data[0].indices[0]));
        d3dxmesh->lpVtbl->UnlockIndexBuffer(d3dxmesh);

        if (i == 0) {
            hr = d3dxmesh->lpVtbl->GenerateAdjacency(d3dxmesh, 0.0f, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
        }

        hr = d3dxmesh->lpVtbl->GenerateAdjacency(d3dxmesh, test_data[i].epsilon, adjacency);
        ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
        if (FAILED(hr)) continue;

        for (j = 0; j < test_data[i].num_faces * 3; j++)
            ok(adjacency[j] == test_data[i].adjacency[j],
               "Test %d adjacency %d: Got result %lu, expected %lu\n", i, j,
               adjacency[j], test_data[i].adjacency[j]);
    }
    if (d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);

    free_test_context(test_context);
}

static void test_update_semantics(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    ID3DXMesh *mesh = NULL;
    D3DVERTEXELEMENT9 declaration0[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_pos_type_color[] =
    {
         {0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_smaller[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_larger[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         {0, 40, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_multiple_streams[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},

         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_double_usage[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_undefined_type[] =
    {
         {0, 0, D3DDECLTYPE_UNUSED+1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_not_4_byte_aligned_offset[] =
    {
         {0, 3, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    static const struct
    {
        D3DXVECTOR3 position0;
        D3DXVECTOR3 position1;
        D3DXVECTOR3 normal;
        DWORD color;
    }
    vertices[] =
    {
        { { 0.0f,  1.0f,  0.f}, { 1.0f,  0.0f,  0.f}, {0.0f, 0.0f, 1.0f}, 0xffff0000 },
        { { 1.0f, -1.0f,  0.f}, {-1.0f, -1.0f,  0.f}, {0.0f, 0.0f, 1.0f}, 0xff00ff00 },
        { {-1.0f, -1.0f,  0.f}, {-1.0f,  1.0f,  0.f}, {0.0f, 0.0f, 1.0f}, 0xff0000ff },
    };
    unsigned int faces[] = {0, 1, 2};
    unsigned int attributes[] = {0};
    unsigned int num_faces = ARRAY_SIZE(faces) / 3;
    unsigned int num_vertices = ARRAY_SIZE(vertices);
    int offset = sizeof(D3DXVECTOR3);
    DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    void *vertex_buffer;
    void *index_buffer;
    DWORD *attributes_buffer;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];
    D3DVERTEXELEMENT9 *decl_ptr;
    DWORD exp_vertex_size = sizeof(*vertices);
    DWORD vertex_size = 0;
    BYTE *decl_mem;
    int equal;
    int i = 0;

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create a test_context\n");
        goto cleanup;
    }

    hr = D3DXCreateMesh(num_faces, num_vertices, options, declaration0,
                        test_context->device, &mesh);
    if (FAILED(hr))
    {
        skip("Couldn't create test mesh %#lx\n", hr);
        goto cleanup;
    }

    mesh->lpVtbl->LockVertexBuffer(mesh, 0, &vertex_buffer);
    memcpy(vertex_buffer, vertices, sizeof(vertices));
    mesh->lpVtbl->UnlockVertexBuffer(mesh);

    mesh->lpVtbl->LockIndexBuffer(mesh, 0, &index_buffer);
    memcpy(index_buffer, faces, sizeof(faces));
    mesh->lpVtbl->UnlockIndexBuffer(mesh);

    mesh->lpVtbl->LockAttributeBuffer(mesh, 0, &attributes_buffer);
    memcpy(attributes_buffer, attributes, sizeof(attributes));
    mesh->lpVtbl->UnlockAttributeBuffer(mesh);

    /* Get the declaration and try to change it */
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    if (FAILED(hr))
    {
        skip("Couldn't get vertex declaration %#lx\n", hr);
        goto cleanup;
    }
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    for (decl_ptr = declaration; decl_ptr->Stream != 0xFF; decl_ptr++)
    {
        if (decl_ptr->Usage == D3DDECLUSAGE_POSITION)
        {
            /* Use second vertex position instead of first */
            decl_ptr->Offset = offset;
        }
    }

    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration);
    ok(hr == D3D_OK, "Test UpdateSemantics, got %#lx expected %#lx\n", hr, D3D_OK);

    /* Check that declaration was written by getting it again */
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    if (FAILED(hr))
    {
        skip("Couldn't get vertex declaration %#lx\n", hr);
        goto cleanup;
    }

    for (decl_ptr = declaration; decl_ptr->Stream != 0xFF; decl_ptr++)
    {
        if (decl_ptr->Usage == D3DDECLUSAGE_POSITION)
        {
            ok(decl_ptr->Offset == offset, "Test UpdateSemantics, got offset %d expected %d\n",
               decl_ptr->Offset, offset);
        }
    }

    /* Check that GetDeclaration only writes up to the D3DDECL_END() marker and
     * not the full MAX_FVF_DECL_SIZE elements.
     */
    memset(declaration, 0xaa, sizeof(declaration));
    memcpy(declaration, declaration0, sizeof(declaration0));
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration);
    ok(hr == D3D_OK, "Test UpdateSemantics, "
       "got %#lx expected D3D_OK\n", hr);
    memset(declaration, 0xbb, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    decl_mem = (BYTE *)declaration;
    for (i = sizeof(declaration0); i < sizeof(declaration); ++i)
    {
        ok(decl_mem[i] == 0xbb, "Unexpected %#x.\n", decl_mem[i]);
        if (equal != 0)
            break;
    }

    /* UpdateSemantics does not check for overlapping fields */
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    if (FAILED(hr))
    {
        skip("Couldn't get vertex declaration %#lx\n", hr);
        goto cleanup;
    }

    for (decl_ptr = declaration; decl_ptr->Stream != 0xFF; decl_ptr++)
    {
        if (decl_ptr->Type == D3DDECLTYPE_FLOAT3)
        {
            decl_ptr->Type = D3DDECLTYPE_FLOAT4;
        }
    }

    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration);
    ok(hr == D3D_OK, "Test UpdateSemantics for overlapping fields, "
       "got %#lx expected D3D_OK\n", hr);

    /* Set the position type to color instead of float3 */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_pos_type_color);
    ok(hr == D3D_OK, "Test UpdateSemantics position type color, "
       "got %#lx expected D3D_OK\n", hr);

    /* The following test cases show that NULL, smaller or larger declarations,
     * and declarations with non-zero Stream values are not accepted.
     * UpdateSemantics returns D3DERR_INVALIDCALL and the previously set
     * declaration will be used by DrawSubset, GetNumBytesPerVertex, and
     * GetDeclaration.
     */

    /* Null declaration (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSemantics null pointer declaration, "
       "got %#lx expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %lu, expected %lu\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Smaller vertex declaration (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_smaller);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSemantics for smaller vertex declaration, "
       "got %#lx expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %lu, expected %lu\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Larger vertex declaration (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_larger);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSemantics for larger vertex declaration, "
       "got %#lx expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %lu, expected %lu\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Use multiple streams and keep the same vertex size (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_multiple_streams);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSemantics using multiple streams, "
                 "got %#lx expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %lu, expected %lu\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* The next following test cases show that some invalid declarations are
     * accepted with a D3D_OK. An access violation is thrown on Windows if
     * DrawSubset is called. The methods GetNumBytesPerVertex and GetDeclaration
     * are not affected, which indicates that the declaration is cached.
     */

    /* Double usage (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_double_usage);
    ok(hr == D3D_OK, "Test UpdateSemantics double usage, "
       "got %#lx expected D3D_OK\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %lu, expected %lu\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration_double_usage, sizeof(declaration_double_usage));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Set the position to an undefined type (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_undefined_type);
    ok(hr == D3D_OK, "Test UpdateSemantics undefined type, "
       "got %#lx expected D3D_OK\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %lu, expected %lu\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration_undefined_type, sizeof(declaration_undefined_type));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Use a not 4 byte aligned offset (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_not_4_byte_aligned_offset);
    ok(hr == D3D_OK, "Test UpdateSemantics not 4 byte aligned offset, "
       "got %#lx expected D3D_OK\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %lu, expected %lu\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#lx, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration_not_4_byte_aligned_offset,
                   sizeof(declaration_not_4_byte_aligned_offset));
    ok(equal == 0, "Vertex declarations were not equal\n");

cleanup:
    if (mesh)
        mesh->lpVtbl->Release(mesh);

    free_test_context(test_context);
}

static void test_create_skin_info(void)
{
    D3DVERTEXELEMENT9 empty_declaration[] = { D3DDECL_END() };
    D3DVERTEXELEMENT9 declaration_out[MAX_FVF_DECL_SIZE];
    const D3DVERTEXELEMENT9 declaration_with_nonzero_stream[] = {
        {1, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];
    DWORD exp_vertices[2], vertices[2];
    float exp_weights[2], weights[2];
    const char *exp_string, *string;
    ID3DXSkinInfo *skininfo = NULL;
    DWORD exp_fvf, fvf;
    unsigned int i;
    HRESULT hr;

    hr = D3DXCreateSkinInfo(0, empty_declaration, 0, &skininfo);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    if (skininfo) IUnknown_Release(skininfo);
    skininfo = NULL;

    hr = D3DXCreateSkinInfo(1, NULL, 1, &skininfo);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXCreateSkinInfo(1, declaration_with_nonzero_stream, 1, &skininfo);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXCreateSkinInfoFVF(1, 0, 1, &skininfo);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
    if (skininfo)
    {
        ID3DXSkinInfo *clone = NULL;
        DWORD dword_result;
        float flt_result;
        const char *string_result;
        D3DXMATRIX *transform;
        D3DXMATRIX identity_matrix;

        /* test initial values */
        hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
        if (SUCCEEDED(hr))
            compare_elements(declaration_out, empty_declaration, __LINE__, 0);

        dword_result = skininfo->lpVtbl->GetNumBones(skininfo);
        ok(dword_result == 1, "Expected 1, got %lu\n", dword_result);

        flt_result = skininfo->lpVtbl->GetMinBoneInfluence(skininfo);
        ok(flt_result == 0.0f, "Expected 0.0, got %g\n", flt_result);

        string_result = skininfo->lpVtbl->GetBoneName(skininfo, 0);
        ok(string_result == NULL, "Expected NULL, got %p\n", string_result);

        dword_result = skininfo->lpVtbl->GetFVF(skininfo);
        ok(dword_result == 0, "Expected 0, got %lu\n", dword_result);

        dword_result = skininfo->lpVtbl->GetNumBoneInfluences(skininfo, 0);
        ok(dword_result == 0, "Expected 0, got %lu\n", dword_result);

        dword_result = skininfo->lpVtbl->GetNumBoneInfluences(skininfo, 1);
        ok(dword_result == 0, "Expected 0, got %lu\n", dword_result);

        transform = skininfo->lpVtbl->GetBoneOffsetMatrix(skininfo, -1);
        ok(transform == NULL, "Expected NULL, got %p\n", transform);

        hr = skininfo->lpVtbl->Clone(skininfo, &clone);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
        IUnknown_Release(clone);

        {
            /* test [GS]etBoneOffsetMatrix */
            hr = skininfo->lpVtbl->SetBoneOffsetMatrix(skininfo, 1, &identity_matrix);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetBoneOffsetMatrix(skininfo, 0, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            D3DXMatrixIdentity(&identity_matrix);
            hr = skininfo->lpVtbl->SetBoneOffsetMatrix(skininfo, 0, &identity_matrix);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);

            transform = skininfo->lpVtbl->GetBoneOffsetMatrix(skininfo, 0);
            check_matrix(transform, &identity_matrix);
        }

        {
            /* test [GS]etBoneName */
            const char *name_in = "testBoneName";
            const char *string_result2;

            hr = skininfo->lpVtbl->SetBoneName(skininfo, 1, name_in);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetBoneName(skininfo, 0, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetBoneName(skininfo, 0, name_in);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);

            string_result = skininfo->lpVtbl->GetBoneName(skininfo, 0);
            ok(string_result != NULL, "Expected non-NULL string, got %p\n", string_result);
            ok(!strcmp(string_result, name_in), "Expected '%s', got '%s'\n", name_in, string_result);

            string_result2 = skininfo->lpVtbl->GetBoneName(skininfo, 0);
            ok(string_result == string_result2, "Expected %p, got %p\n", string_result, string_result2);

            string_result = skininfo->lpVtbl->GetBoneName(skininfo, 1);
            ok(string_result == NULL, "Expected NULL, got %p\n", string_result);
        }

        {
            /* test [GS]etBoneInfluence */
            DWORD num_influences;

            /* vertex and weight arrays untouched when num_influences is 0 */
            vertices[0] = 0xdeadbeef;
            weights[0] = FLT_MAX;
            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            ok(vertices[0] == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", vertices[0]);
            ok(weights[0] == FLT_MAX, "expected %g, got %g\n", FLT_MAX, weights[0]);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 1, vertices, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, NULL, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, NULL);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, NULL, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);


            /* no vertex or weight value checking */
            exp_vertices[0] = 0;
            exp_vertices[1] = 0x87654321;
            exp_weights[0] = 0.5;
            exp_weights[1] = NAN;
            num_influences = 2;

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 1, num_influences, vertices, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, NULL, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, vertices, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, NULL, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, exp_vertices, exp_weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);

            memset(vertices, 0, sizeof(vertices));
            memset(weights, 0, sizeof(weights));
            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            for (i = 0; i < num_influences; i++) {
                ok(exp_vertices[i] == vertices[i],
                   "influence[%d]: expected vertex %lu, got %lu\n", i, exp_vertices[i], vertices[i]);
                ok((isnan(exp_weights[i]) && isnan(weights[i])) || exp_weights[i] == weights[i],
                   "influence[%d]: expected weights %g, got %g\n", i, exp_weights[i], weights[i]);
            }

            /* vertices and weights aren't returned after setting num_influences to 0 */
            memset(vertices, 0, sizeof(vertices));
            memset(weights, 0, sizeof(weights));
            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);

            vertices[0] = 0xdeadbeef;
            weights[0] = FLT_MAX;
            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            ok(vertices[0] == 0xdeadbeef, "expected vertex 0xdeadbeef, got %lu\n", vertices[0]);
            ok(weights[0] == FLT_MAX, "expected weight %g, got %g\n", FLT_MAX, weights[0]);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, exp_vertices, exp_weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
        }

        {
            /* test [GS]etFVF and [GS]etDeclaration */
            D3DVERTEXELEMENT9 declaration_in[MAX_FVF_DECL_SIZE];
            DWORD got_fvf;

            fvf = D3DFVF_XYZ;
            hr = skininfo->lpVtbl->SetDeclaration(skininfo, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetDeclaration(skininfo, declaration_with_nonzero_stream);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

            hr = skininfo->lpVtbl->SetFVF(skininfo, 0);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);

            hr = D3DXDeclaratorFromFVF(fvf, declaration_in);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            hr = skininfo->lpVtbl->SetDeclaration(skininfo, declaration_in);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            got_fvf = skininfo->lpVtbl->GetFVF(skininfo);
            ok(fvf == got_fvf, "Expected %#lx, got %#lx\n", fvf, got_fvf);
            hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            compare_elements(declaration_out, declaration_in, __LINE__, 0);

            hr = skininfo->lpVtbl->SetDeclaration(skininfo, empty_declaration);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            got_fvf = skininfo->lpVtbl->GetFVF(skininfo);
            ok(got_fvf == 0, "Expected 0, got %#lx\n", got_fvf);
            hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            compare_elements(declaration_out, empty_declaration, __LINE__, 0);

            hr = skininfo->lpVtbl->SetFVF(skininfo, fvf);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            got_fvf = skininfo->lpVtbl->GetFVF(skininfo);
            ok(fvf == got_fvf, "Expected %#lx, got %#lx\n", fvf, got_fvf);
            hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
            compare_elements(declaration_out, declaration_in, __LINE__, 0);
        }

        /* Test Clone() */
        hr = skininfo->lpVtbl->Clone(skininfo, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

        clone = NULL;
        hr = skininfo->lpVtbl->Clone(skininfo, &clone);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = clone->lpVtbl->GetDeclaration(clone, declaration);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        compare_elements(declaration, declaration_out, __LINE__, 0);

        hr = D3DXFVFFromDeclarator(declaration_out, &exp_fvf);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        fvf = clone->lpVtbl->GetFVF(clone);
        ok(fvf == exp_fvf, "Got unexpected fvf %#lx.\n", fvf);

        exp_string = skininfo->lpVtbl->GetBoneName(skininfo, 0);
        string = clone->lpVtbl->GetBoneName(clone, 0);
        ok(!strcmp(string, exp_string), "Got unexpected bone 0 name %s.\n", debugstr_a(string));

        transform = clone->lpVtbl->GetBoneOffsetMatrix(clone, 0);
        check_matrix(transform, &identity_matrix);

        hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, exp_vertices, exp_weights);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        hr = clone->lpVtbl->GetBoneInfluence(clone, 0, vertices, weights);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        for (i = 0; i < ARRAY_SIZE(vertices); ++i)
        {
            ok(vertices[i] == exp_vertices[i], "influence[%u]: got unexpected vertex %lu, expected %lu.\n",
                    i, vertices[i], exp_vertices[i]);
            ok(((DWORD *)weights)[i] == ((DWORD *)exp_weights)[i],
                    "influence[%u]: got unexpected weight %.8e, expected %.8e.\n", i, weights[i], exp_weights[i]);
        }

        IUnknown_Release(clone);
    }
    if (skininfo) IUnknown_Release(skininfo);
    skininfo = NULL;

    hr = D3DXCreateSkinInfoFVF(1, D3DFVF_XYZ, 1, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);

    hr = D3DXCreateSkinInfo(1, NULL, 1, &skininfo);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#lx\n", hr);
}

static void test_convert_adjacency_to_point_reps(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    const DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    const DWORD options_16bit = D3DXMESH_SYSTEMMEM;
    const D3DVERTEXELEMENT9 declaration[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };
    const unsigned int VERTS_PER_FACE = 3;
    void *vertex_buffer;
    void *index_buffer;
    DWORD *attributes_buffer;
    int i, j;
    enum color { RED = 0xffff0000, GREEN = 0xff00ff00, BLUE = 0xff0000ff};
    struct vertex_pnc
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        enum color color; /* In case of manual visual inspection */
    };
    D3DXVECTOR3 up = {0.0f, 0.0f, 1.0f};
    /* mesh0 (one face)
     *
     * 0--1
     * | /
     * |/
     * 2
     */
    const struct vertex_pnc vertices0[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices0[] = {0, 1, 2};
    const unsigned int num_vertices0 = ARRAY_SIZE(vertices0);
    const unsigned int num_faces0 = ARRAY_SIZE(indices0) / VERTS_PER_FACE;
    const DWORD adjacency0[] = {-1, -1, -1};
    const DWORD exp_point_rep0[] = {0, 1, 2};
    /* mesh1 (right)
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_pnc vertices1[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices1[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices1 = ARRAY_SIZE(vertices1);
    const unsigned int num_faces1 = ARRAY_SIZE(indices1) / VERTS_PER_FACE;
    const DWORD adjacency1[] = {-1, 1, -1, -1, -1, 0};
    const DWORD exp_point_rep1[] = {0, 1, 2, 1, 4, 2};
    /* mesh2 (left)
     *
     *    3 0--1
     *   /| | /
     *  / | |/
     * 5--4 2
     */
    const struct vertex_pnc vertices2[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{-1.0f,  3.0f,  0.f}, up, RED},
        {{-1.0f,  0.0f,  0.f}, up, GREEN},
        {{-3.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices2[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices2 = ARRAY_SIZE(vertices2);
    const unsigned int num_faces2 = ARRAY_SIZE(indices2) / VERTS_PER_FACE;
    const DWORD adjacency2[] = {-1, -1, 1, 0, -1, -1};
    const DWORD exp_point_rep2[] = {0, 1, 2, 0, 2, 5};
    /* mesh3 (above)
     *
     *    3
     *   /|
     *  / |
     * 5--4
     * 0--1
     * | /
     * |/
     * 2
     */
    struct vertex_pnc vertices3[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 2.0f,  7.0f,  0.f}, up, BLUE},
        {{ 2.0f,  4.0f,  0.f}, up, GREEN},
        {{ 0.0f,  4.0f,  0.f}, up, RED},
    };
    const DWORD indices3[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices3 = ARRAY_SIZE(vertices3);
    const unsigned int num_faces3 = ARRAY_SIZE(indices3) / VERTS_PER_FACE;
    const DWORD adjacency3[] = {1, -1, -1, -1, 0, -1};
    const DWORD exp_point_rep3[] = {0, 1, 2, 3, 1, 0};
    /* mesh4 (below, tip against tip)
     *
     * 0--1
     * | /
     * |/
     * 2
     * 3
     * |\
     * | \
     * 5--4
     */
    struct vertex_pnc vertices4[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 0.0f, -4.0f,  0.f}, up, BLUE},
        {{ 2.0f, -7.0f,  0.f}, up, GREEN},
        {{ 0.0f, -7.0f,  0.f}, up, RED},
    };
    const DWORD indices4[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices4 = ARRAY_SIZE(vertices4);
    const unsigned int num_faces4 = ARRAY_SIZE(indices4) / VERTS_PER_FACE;
    const DWORD adjacency4[] = {-1, -1, -1, -1, -1, -1};
    const DWORD exp_point_rep4[] = {0, 1, 2, 3, 4, 5};
    /* mesh5 (gap in mesh)
     *
     *    0      3-----4  15
     *   / \      \   /  /  \
     *  /   \      \ /  /    \
     * 2-----1      5 17-----16
     * 6-----7      9 12-----13
     *  \   /      / \  \    /
     *   \ /      /   \  \  /
     *    8     10-----11 14
     *
     */
    const struct vertex_pnc vertices5[] =
    {
        {{ 0.0f,  1.0f,  0.f}, up, RED},
        {{ 1.0f, -1.0f,  0.f}, up, GREEN},
        {{-1.0f, -1.0f,  0.f}, up, BLUE},

        {{ 0.1f,  1.0f,  0.f}, up, RED},
        {{ 2.1f,  1.0f,  0.f}, up, BLUE},
        {{ 1.1f, -1.0f,  0.f}, up, GREEN},

        {{-1.0f, -1.1f,  0.f}, up, BLUE},
        {{ 1.0f, -1.1f,  0.f}, up, GREEN},
        {{ 0.0f, -3.1f,  0.f}, up, RED},

        {{ 1.1f, -1.1f,  0.f}, up, GREEN},
        {{ 2.1f, -3.1f,  0.f}, up, BLUE},
        {{ 0.1f, -3.1f,  0.f}, up, RED},

        {{ 1.2f, -1.1f,  0.f}, up, GREEN},
        {{ 3.2f, -1.1f,  0.f}, up, RED},
        {{ 2.2f, -3.1f,  0.f}, up, BLUE},

        {{ 2.2f,  1.0f,  0.f}, up, BLUE},
        {{ 3.2f, -1.0f,  0.f}, up, RED},
        {{ 1.2f, -1.0f,  0.f}, up, GREEN},
    };
    const DWORD indices5[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    const unsigned int num_vertices5 = ARRAY_SIZE(vertices5);
    const unsigned int num_faces5 = ARRAY_SIZE(indices5) / VERTS_PER_FACE;
    const DWORD adjacency5[] = {-1, 2, -1, -1, 5, -1, 0, -1, -1, 4, -1, -1, 5, -1, 3, -1, 4, 1};
    const DWORD exp_point_rep5[] = {0, 1, 2, 3, 4, 5, 2, 1, 8, 5, 10, 11, 5, 13, 10, 4, 13, 5};
    const WORD indices5_16bit[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    /* mesh6 (indices re-ordering)
     *
     * 0--1 6 3
     * | / /| |\
     * |/ / | | \
     * 2 8--7 5--4
     */
    const struct vertex_pnc vertices6[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},

        {{ 4.0f,  3.0f,  0.f}, up, GREEN},
        {{ 6.0f,  0.0f,  0.f}, up, BLUE},
        {{ 4.0f,  0.0f,  0.f}, up, RED},
    };
    const DWORD indices6[] = {0, 1, 2, 6, 7, 8, 3, 4, 5};
    const unsigned int num_vertices6 = ARRAY_SIZE(vertices6);
    const unsigned int num_faces6 = ARRAY_SIZE(indices6) / VERTS_PER_FACE;
    const DWORD adjacency6[] = {-1, 1, -1, 2, -1, 0, -1, -1, 1};
    const DWORD exp_point_rep6[] = {0, 1, 2, 1, 4, 5, 1, 5, 2};
    /* mesh7 (expands collapsed triangle)
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_pnc vertices7[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices7[] = {0, 1, 2, 3, 3, 3}; /* Face 1 is collapsed*/
    const unsigned int num_vertices7 = ARRAY_SIZE(vertices7);
    const unsigned int num_faces7 = ARRAY_SIZE(indices7) / VERTS_PER_FACE;
    const DWORD adjacency7[] = {-1, -1, -1, -1, -1, -1};
    const DWORD exp_point_rep7[] = {0, 1, 2, 3, 4, 5};
    /* mesh8 (indices re-ordering and double replacement)
     *
     * 0--1 9  6
     * | / /|  |\
     * |/ / |  | \
     * 2 11-10 8--7
     *         3--4
     *         | /
     *         |/
     *         5
     */
    const struct vertex_pnc vertices8[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 4.0,  -4.0,  0.f}, up, RED},
        {{ 6.0,  -4.0,  0.f}, up, BLUE},
        {{ 4.0,  -7.0,  0.f}, up, GREEN},

        {{ 4.0f,  3.0f,  0.f}, up, GREEN},
        {{ 6.0f,  0.0f,  0.f}, up, BLUE},
        {{ 4.0f,  0.0f,  0.f}, up, RED},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices8[] = {0, 1, 2, 9, 10, 11, 6, 7, 8, 3, 4, 5};
    const unsigned int num_vertices8 = ARRAY_SIZE(vertices8);
    const unsigned int num_faces8 = ARRAY_SIZE(indices8) / VERTS_PER_FACE;
    const DWORD adjacency8[] = {-1, 1, -1, 2, -1, 0, -1, 3, 1, 2, -1, -1};
    const DWORD exp_point_rep8[] = {0, 1, 2, 3, 4, 5, 1, 4, 3, 1, 3, 2};
    /* mesh9 (right, shared vertices)
     *
     * 0--1
     * | /|
     * |/ |
     * 2--3
     */
    const struct vertex_pnc vertices9[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 2.0f,  0.0f,  0.f}, up, RED},
    };
    const DWORD indices9[] = {0, 1, 2, 1, 3, 2};
    const unsigned int num_vertices9 = ARRAY_SIZE(vertices9);
    const unsigned int num_faces9 = ARRAY_SIZE(indices9) / VERTS_PER_FACE;
    const DWORD adjacency9[] = {-1, 1, -1, -1, -1, 0};
    const DWORD exp_point_rep9[] = {0, 1, 2, 3};
    /* All mesh data */
    ID3DXMesh *mesh = NULL;
    ID3DXMesh *mesh_null_check = NULL;
    unsigned int attributes[] = {0};
    struct
    {
        const struct vertex_pnc *vertices;
        const DWORD *indices;
        const DWORD num_vertices;
        const DWORD num_faces;
        const DWORD *adjacency;
        const DWORD *exp_point_reps;
        const DWORD options;
    }
    tc[] =
    {
        {
            vertices0,
            indices0,
            num_vertices0,
            num_faces0,
            adjacency0,
            exp_point_rep0,
            options
        },
        {
            vertices1,
            indices1,
            num_vertices1,
            num_faces1,
            adjacency1,
            exp_point_rep1,
            options
        },
        {
            vertices2,
            indices2,
            num_vertices2,
            num_faces2,
            adjacency2,
            exp_point_rep2,
            options
        },
        {
            vertices3,
            indices3,
            num_vertices3,
            num_faces3,
            adjacency3,
            exp_point_rep3,
            options
        },
        {
            vertices4,
            indices4,
            num_vertices4,
            num_faces4,
            adjacency4,
            exp_point_rep4,
            options
        },
        {
            vertices5,
            indices5,
            num_vertices5,
            num_faces5,
            adjacency5,
            exp_point_rep5,
            options
        },
        {
            vertices6,
            indices6,
            num_vertices6,
            num_faces6,
            adjacency6,
            exp_point_rep6,
            options
        },
        {
            vertices7,
            indices7,
            num_vertices7,
            num_faces7,
            adjacency7,
            exp_point_rep7,
            options
        },
        {
            vertices8,
            indices8,
            num_vertices8,
            num_faces8,
            adjacency8,
            exp_point_rep8,
            options
        },
        {
            vertices9,
            indices9,
            num_vertices9,
            num_faces9,
            adjacency9,
            exp_point_rep9,
            options
        },
        {
            vertices5,
            (DWORD*)indices5_16bit,
            num_vertices5,
            num_faces5,
            adjacency5,
            exp_point_rep5,
            options_16bit
        },
    };
    DWORD *point_reps = NULL;

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        goto cleanup;
    }

    for (i = 0; i < ARRAY_SIZE(tc); i++)
    {
        hr = D3DXCreateMesh(tc[i].num_faces, tc[i].num_vertices, tc[i].options, declaration,
                            test_context->device, &mesh);
        if (FAILED(hr))
        {
            skip("Couldn't create mesh %d. Got %lx expected D3D_OK\n", i, hr);
            goto cleanup;
        }

        if (i == 0) /* Save first mesh for later NULL checks */
            mesh_null_check = mesh;

        point_reps = malloc(tc[i].num_vertices * sizeof(*point_reps));
        if (!point_reps)
        {
            skip("Couldn't allocate point reps array.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, &vertex_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock vertex buffer.\n");
            goto cleanup;
        }
        memcpy(vertex_buffer, tc[i].vertices, tc[i].num_vertices * sizeof(*tc[i].vertices));
        hr = mesh->lpVtbl->UnlockVertexBuffer(mesh);
        if (FAILED(hr))
        {
            skip("Couldn't unlock vertex buffer.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, &index_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock index buffer.\n");
            goto cleanup;
        }
        if (tc[i].options & D3DXMESH_32BIT)
        {
            memcpy(index_buffer, tc[i].indices, VERTS_PER_FACE * tc[i].num_faces * sizeof(DWORD));
        }
        else
        {
            memcpy(index_buffer, tc[i].indices, VERTS_PER_FACE * tc[i].num_faces * sizeof(WORD));
        }
        hr = mesh->lpVtbl->UnlockIndexBuffer(mesh);
        if (FAILED(hr)) {
            skip("Couldn't unlock index buffer.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockAttributeBuffer(mesh, 0, &attributes_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock attributes buffer.\n");
            goto cleanup;
        }
        memcpy(attributes_buffer, attributes, sizeof(attributes));
        hr = mesh->lpVtbl->UnlockAttributeBuffer(mesh);
        if (FAILED(hr))
        {
            skip("Couldn't unlock attributes buffer.\n");
            goto cleanup;
        }

        /* Convert adjacency to point representation */
        for (j = 0; j < tc[i].num_vertices; j++) point_reps[j] = -1;
        hr = mesh->lpVtbl->ConvertAdjacencyToPointReps(mesh, tc[i].adjacency, point_reps);
        ok(hr == D3D_OK, "ConvertAdjacencyToPointReps failed case %d. "
           "Got %lx expected D3D_OK\n", i, hr);

        /* Check point representation */
        for (j = 0; j < tc[i].num_vertices; j++)
        {
            ok(point_reps[j] == tc[i].exp_point_reps[j],
               "Unexpected point representation at (%d, %d)."
               " Got %ld expected %ld\n",
               i, j, point_reps[j], tc[i].exp_point_reps[j]);
        }

        free(point_reps);
        point_reps = NULL;

        if (i != 0) /* First mesh will be freed during cleanup */
            mesh->lpVtbl->Release(mesh);
    }

    /* NULL checks */
    hr = mesh_null_check->lpVtbl->ConvertAdjacencyToPointReps(mesh_null_check, tc[0].adjacency, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ConvertAdjacencyToPointReps point_reps NULL. "
       "Got %lx expected D3DERR_INVALIDCALL\n", hr);
    hr = mesh_null_check->lpVtbl->ConvertAdjacencyToPointReps(mesh_null_check, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ConvertAdjacencyToPointReps adjacency and point_reps NULL. "
       "Got %lx expected D3DERR_INVALIDCALL\n", hr);

cleanup:
    if (mesh_null_check)
        mesh_null_check->lpVtbl->Release(mesh_null_check);
    free(point_reps);
    free_test_context(test_context);
}

static void test_convert_point_reps_to_adjacency(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    const DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    const DWORD options_16bit = D3DXMESH_SYSTEMMEM;
    const D3DVERTEXELEMENT9 declaration[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };
    const unsigned int VERTS_PER_FACE = 3;
    void *vertex_buffer;
    void *index_buffer;
    DWORD *attributes_buffer;
    int i, j;
    enum color { RED = 0xffff0000, GREEN = 0xff00ff00, BLUE = 0xff0000ff};
    struct vertex_pnc
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        enum color color; /* In case of manual visual inspection */
    };
    D3DXVECTOR3 up = {0.0f, 0.0f, 1.0f};
    /* mesh0 (one face)
     *
     * 0--1
     * | /
     * |/
     * 2
     */
    const struct vertex_pnc vertices0[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices0[] = {0, 1, 2};
    const unsigned int num_vertices0 = ARRAY_SIZE(vertices0);
    const unsigned int num_faces0 = num_vertices0 / VERTS_PER_FACE;
    const DWORD exp_adjacency0[] = {-1, -1, -1};
    const DWORD exp_id_adjacency0[] = {-1, -1, -1};
    const DWORD point_rep0[] = {0, 1, 2};
    /* mesh1 (right)
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_pnc vertices1[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices1[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices1 = ARRAY_SIZE(vertices1);
    const unsigned int num_faces1 = num_vertices1 / VERTS_PER_FACE;
    const DWORD exp_adjacency1[] = {-1, 1, -1, -1, -1, 0};
    const DWORD exp_id_adjacency1[] = {-1, -1, -1, -1, -1, -1};
    const DWORD point_rep1[] = {0, 1, 2, 1, 4, 2};
    /* mesh2 (left)
     *
     *    3 0--1
     *   /| | /
     *  / | |/
     * 5--4 2
     */
    const struct vertex_pnc vertices2[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{-1.0f,  3.0f,  0.f}, up, RED},
        {{-1.0f,  0.0f,  0.f}, up, GREEN},
        {{-3.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices2[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices2 = ARRAY_SIZE(vertices2);
    const unsigned int num_faces2 = num_vertices2 / VERTS_PER_FACE;
    const DWORD exp_adjacency2[] = {-1, -1, 1, 0, -1, -1};
    const DWORD exp_id_adjacency2[] = {-1, -1, -1, -1, -1, -1};
    const DWORD point_rep2[] = {0, 1, 2, 0, 2, 5};
    /* mesh3 (above)
     *
     *    3
     *   /|
     *  / |
     * 5--4
     * 0--1
     * | /
     * |/
     * 2
     */
    struct vertex_pnc vertices3[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 2.0f,  7.0f,  0.f}, up, BLUE},
        {{ 2.0f,  4.0f,  0.f}, up, GREEN},
        {{ 0.0f,  4.0f,  0.f}, up, RED},
    };
    const DWORD indices3[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices3 = ARRAY_SIZE(vertices3);
    const unsigned int num_faces3 = num_vertices3 / VERTS_PER_FACE;
    const DWORD exp_adjacency3[] = {1, -1, -1, -1, 0, -1};
    const DWORD exp_id_adjacency3[] = {-1, -1, -1, -1, -1, -1};
    const DWORD point_rep3[] = {0, 1, 2, 3, 1, 0};
    /* mesh4 (below, tip against tip)
     *
     * 0--1
     * | /
     * |/
     * 2
     * 3
     * |\
     * | \
     * 5--4
     */
    struct vertex_pnc vertices4[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 0.0f, -4.0f,  0.f}, up, BLUE},
        {{ 2.0f, -7.0f,  0.f}, up, GREEN},
        {{ 0.0f, -7.0f,  0.f}, up, RED},
    };
    const DWORD indices4[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices4 = ARRAY_SIZE(vertices4);
    const unsigned int num_faces4 = num_vertices4 / VERTS_PER_FACE;
    const DWORD exp_adjacency4[] = {-1, -1, -1, -1, -1, -1};
    const DWORD exp_id_adjacency4[] = {-1, -1, -1, -1, -1, -1};
    const DWORD point_rep4[] = {0, 1, 2, 3, 4, 5};
    /* mesh5 (gap in mesh)
     *
     *    0      3-----4  15
     *   / \      \   /  /  \
     *  /   \      \ /  /    \
     * 2-----1      5 17-----16
     * 6-----7      9 12-----13
     *  \   /      / \  \    /
     *   \ /      /   \  \  /
     *    8     10-----11 14
     *
     */
    const struct vertex_pnc vertices5[] =
    {
        {{ 0.0f,  1.0f,  0.f}, up, RED},
        {{ 1.0f, -1.0f,  0.f}, up, GREEN},
        {{-1.0f, -1.0f,  0.f}, up, BLUE},

        {{ 0.1f,  1.0f,  0.f}, up, RED},
        {{ 2.1f,  1.0f,  0.f}, up, BLUE},
        {{ 1.1f, -1.0f,  0.f}, up, GREEN},

        {{-1.0f, -1.1f,  0.f}, up, BLUE},
        {{ 1.0f, -1.1f,  0.f}, up, GREEN},
        {{ 0.0f, -3.1f,  0.f}, up, RED},

        {{ 1.1f, -1.1f,  0.f}, up, GREEN},
        {{ 2.1f, -3.1f,  0.f}, up, BLUE},
        {{ 0.1f, -3.1f,  0.f}, up, RED},

        {{ 1.2f, -1.1f,  0.f}, up, GREEN},
        {{ 3.2f, -1.1f,  0.f}, up, RED},
        {{ 2.2f, -3.1f,  0.f}, up, BLUE},

        {{ 2.2f,  1.0f,  0.f}, up, BLUE},
        {{ 3.2f, -1.0f,  0.f}, up, RED},
        {{ 1.2f, -1.0f,  0.f}, up, GREEN},
    };
    const DWORD indices5[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    const unsigned int num_vertices5 = ARRAY_SIZE(vertices5);
    const unsigned int num_faces5 = num_vertices5 / VERTS_PER_FACE;
    const DWORD exp_adjacency5[] = {-1, 2, -1, -1, 5, -1, 0, -1, -1, 4, -1, -1, 5, -1, 3, -1, 4, 1};
    const DWORD exp_id_adjacency5[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    const DWORD point_rep5[] = {0, 1, 2, 3, 4, 5, 2, 1, 8, 5, 10, 11, 5, 13, 10, 4, 13, 5};
    /* mesh6 (indices re-ordering)
     *
     * 0--1 6 3
     * | / /| |\
     * |/ / | | \
     * 2 8--7 5--4
     */
    const struct vertex_pnc vertices6[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},

        {{ 4.0f,  3.0f,  0.f}, up, GREEN},
        {{ 6.0f,  0.0f,  0.f}, up, BLUE},
        {{ 4.0f,  0.0f,  0.f}, up, RED},
    };
    const DWORD indices6[] = {0, 1, 2, 6, 7, 8, 3, 4, 5};
    const unsigned int num_vertices6 = ARRAY_SIZE(vertices6);
    const unsigned int num_faces6 = num_vertices6 / VERTS_PER_FACE;
    const DWORD exp_adjacency6[] = {-1, 1, -1, 2, -1, 0, -1, -1, 1};
    const DWORD exp_id_adjacency6[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
    const DWORD point_rep6[] = {0, 1, 2, 1, 4, 5, 1, 5, 2};
    /* mesh7 (expands collapsed triangle)
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_pnc vertices7[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices7[] = {0, 1, 2, 3, 3, 3}; /* Face 1 is collapsed*/
    const unsigned int num_vertices7 = ARRAY_SIZE(vertices7);
    const unsigned int num_faces7 = num_vertices7 / VERTS_PER_FACE;
    const DWORD exp_adjacency7[] = {-1, -1, -1, -1, -1, -1};
    const DWORD exp_id_adjacency7[] = {-1, -1, -1, -1, -1, -1};
    const DWORD point_rep7[] = {0, 1, 2, 3, 4, 5};
    /* mesh8 (indices re-ordering and double replacement)
     *
     * 0--1 9  6
     * | / /|  |\
     * |/ / |  | \
     * 2 11-10 8--7
     *         3--4
     *         | /
     *         |/
     *         5
     */
    const struct vertex_pnc vertices8[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 4.0,  -4.0,  0.f}, up, RED},
        {{ 6.0,  -4.0,  0.f}, up, BLUE},
        {{ 4.0,  -7.0,  0.f}, up, GREEN},

        {{ 4.0f,  3.0f,  0.f}, up, GREEN},
        {{ 6.0f,  0.0f,  0.f}, up, BLUE},
        {{ 4.0f,  0.0f,  0.f}, up, RED},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices8[] = {0, 1, 2, 9, 10, 11, 6, 7, 8, 3, 4, 5};
    const WORD indices8_16bit[] = {0, 1, 2, 9, 10, 11, 6, 7, 8, 3, 4, 5};
    const unsigned int num_vertices8 = ARRAY_SIZE(vertices8);
    const unsigned int num_faces8 = num_vertices8 / VERTS_PER_FACE;
    const DWORD exp_adjacency8[] = {-1, 1, -1, 2, -1, 0, -1, 3, 1, 2, -1, -1};
    const DWORD exp_id_adjacency8[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    const DWORD point_rep8[] = {0, 1, 2, 3, 4, 5, 1, 4, 3, 1, 3, 2};
     /* mesh9 (right, shared vertices)
     *
     * 0--1
     * | /|
     * |/ |
     * 2--3
     */
    const struct vertex_pnc vertices9[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 2.0f,  0.0f,  0.f}, up, RED},
    };
    const DWORD indices9[] = {0, 1, 2, 1, 3, 2};
    const unsigned int num_vertices9 = ARRAY_SIZE(vertices9);
    const unsigned int num_faces9 = 2;
    const DWORD exp_adjacency9[] = {-1, 1, -1, -1, -1, 0};
    const DWORD exp_id_adjacency9[] = {-1, 1, -1, -1, -1, 0};
    const DWORD point_rep9[] = {0, 1, 2, 3};
    /* All mesh data */
    ID3DXMesh *mesh = NULL;
    ID3DXMesh *mesh_null_check = NULL;
    unsigned int attributes[] = {0};
    struct
    {
        const struct vertex_pnc *vertices;
        const DWORD *indices;
        const DWORD num_vertices;
        const DWORD num_faces;
        const DWORD *point_reps;
        const DWORD *exp_adjacency;
        const DWORD *exp_id_adjacency;
        const DWORD options;
    }
    tc[] =
    {
        {
            vertices0,
            indices0,
            num_vertices0,
            num_faces0,
            point_rep0,
            exp_adjacency0,
            exp_id_adjacency0,
            options
        },
        {
            vertices1,
            indices1,
            num_vertices1,
            num_faces1,
            point_rep1,
            exp_adjacency1,
            exp_id_adjacency1,
            options
        },
        {
            vertices2,
            indices2,
            num_vertices2,
            num_faces2,
            point_rep2,
            exp_adjacency2,
            exp_id_adjacency2,
            options
        },
        {
            vertices3,
            indices3,
            num_vertices3,
            num_faces3,
            point_rep3,
            exp_adjacency3,
            exp_id_adjacency3,
            options
        },
        {
            vertices4,
            indices4,
            num_vertices4,
            num_faces4,
            point_rep4,
            exp_adjacency4,
            exp_id_adjacency4,
            options
        },
        {
            vertices5,
            indices5,
            num_vertices5,
            num_faces5,
            point_rep5,
            exp_adjacency5,
            exp_id_adjacency5,
            options
        },
        {
            vertices6,
            indices6,
            num_vertices6,
            num_faces6,
            point_rep6,
            exp_adjacency6,
            exp_id_adjacency6,
            options
        },
        {
            vertices7,
            indices7,
            num_vertices7,
            num_faces7,
            point_rep7,
            exp_adjacency7,
            exp_id_adjacency7,
            options
        },
        {
            vertices8,
            indices8,
            num_vertices8,
            num_faces8,
            point_rep8,
            exp_adjacency8,
            exp_id_adjacency8,
            options
        },
        {
            vertices9,
            indices9,
            num_vertices9,
            num_faces9,
            point_rep9,
            exp_adjacency9,
            exp_id_adjacency9,
            options
        },
        {
            vertices8,
            (DWORD*)indices8_16bit,
            num_vertices8,
            num_faces8,
            point_rep8,
            exp_adjacency8,
            exp_id_adjacency8,
            options_16bit
        },
    };
    DWORD *adjacency = NULL;

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        goto cleanup;
    }

    for (i = 0; i < ARRAY_SIZE(tc); i++)
    {
        hr = D3DXCreateMesh(tc[i].num_faces, tc[i].num_vertices, tc[i].options,
                            declaration, test_context->device, &mesh);
        if (FAILED(hr))
        {
            skip("Couldn't create mesh %d. Got %lx expected D3D_OK\n", i, hr);
            goto cleanup;
        }

        if (i == 0) /* Save first mesh for later NULL checks */
            mesh_null_check = mesh;

        adjacency = malloc(VERTS_PER_FACE * tc[i].num_faces * sizeof(*adjacency));
        if (!adjacency)
        {
            skip("Couldn't allocate adjacency array.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, &vertex_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock vertex buffer.\n");
            goto cleanup;
        }
        memcpy(vertex_buffer, tc[i].vertices, tc[i].num_vertices * sizeof(*tc[i].vertices));
        hr = mesh->lpVtbl->UnlockVertexBuffer(mesh);
        if (FAILED(hr))
        {
            skip("Couldn't unlock vertex buffer.\n");
            goto cleanup;
        }
        hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, &index_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock index buffer.\n");
            goto cleanup;
        }
        if (tc[i].options & D3DXMESH_32BIT)
        {
            memcpy(index_buffer, tc[i].indices, VERTS_PER_FACE * tc[i].num_faces * sizeof(DWORD));
        }
        else
        {
            memcpy(index_buffer, tc[i].indices, VERTS_PER_FACE * tc[i].num_faces * sizeof(WORD));
        }
        hr = mesh->lpVtbl->UnlockIndexBuffer(mesh);
        if (FAILED(hr)) {
            skip("Couldn't unlock index buffer.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockAttributeBuffer(mesh, 0, &attributes_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock attributes buffer.\n");
            goto cleanup;
        }
        memcpy(attributes_buffer, attributes, sizeof(attributes));
        hr = mesh->lpVtbl->UnlockAttributeBuffer(mesh);
        if (FAILED(hr))
        {
            skip("Couldn't unlock attributes buffer.\n");
            goto cleanup;
        }

        /* Convert point representation to adjacency*/
        for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++) adjacency[j] = -2;

        hr = mesh->lpVtbl->ConvertPointRepsToAdjacency(mesh, tc[i].point_reps, adjacency);
        ok(hr == D3D_OK, "ConvertPointRepsToAdjacency failed case %d. "
           "Got %lx expected D3D_OK\n", i, hr);
        /* Check adjacency */
        for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++)
        {
            ok(adjacency[j] == tc[i].exp_adjacency[j],
               "Unexpected adjacency information at (%d, %d)."
               " Got %ld expected %ld\n",
               i, j, adjacency[j], tc[i].exp_adjacency[j]);
        }

        /* NULL point representation is considered identity. */
        for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++) adjacency[j] = -2;
        hr = mesh_null_check->lpVtbl->ConvertPointRepsToAdjacency(mesh, NULL, adjacency);
        ok(hr == D3D_OK, "ConvertPointRepsToAdjacency NULL point_reps. "
                     "Got %lx expected D3D_OK\n", hr);
        for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++)
        {
            ok(adjacency[j] == tc[i].exp_id_adjacency[j],
               "Unexpected adjacency information (id) at (%d, %d)."
               " Got %ld expected %ld\n",
               i, j, adjacency[j], tc[i].exp_id_adjacency[j]);
        }

        free(adjacency);
        adjacency = NULL;
        if (i != 0) /* First mesh will be freed during cleanup */
            mesh->lpVtbl->Release(mesh);
    }

    /* NULL checks */
    hr = mesh_null_check->lpVtbl->ConvertPointRepsToAdjacency(mesh_null_check, tc[0].point_reps, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ConvertPointRepsToAdjacency NULL adjacency. "
       "Got %lx expected D3DERR_INVALIDCALL\n", hr);
    hr = mesh_null_check->lpVtbl->ConvertPointRepsToAdjacency(mesh_null_check, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ConvertPointRepsToAdjacency NULL point_reps and adjacency. "
       "Got %lx expected D3DERR_INVALIDCALL\n", hr);

cleanup:
    if (mesh_null_check)
        mesh_null_check->lpVtbl->Release(mesh_null_check);
    free(adjacency);
    free_test_context(test_context);
}

static HRESULT init_test_mesh(const DWORD num_faces, const DWORD num_vertices,
                              const DWORD options,
                              const D3DVERTEXELEMENT9 *declaration,
                              IDirect3DDevice9 *device, ID3DXMesh **mesh_ptr,
                              const void *vertices, const DWORD vertex_size,
                              const DWORD *indices, const DWORD *attributes)
{
    HRESULT hr;
    void *vertex_buffer;
    void *index_buffer;
    DWORD *attributes_buffer;
    ID3DXMesh *mesh = NULL;

    hr = D3DXCreateMesh(num_faces, num_vertices, options, declaration, device, mesh_ptr);
    if (FAILED(hr))
    {
        skip("Couldn't create mesh. Got %lx expected D3D_OK\n", hr);
        goto cleanup;
    }
    mesh = *mesh_ptr;

    hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, &vertex_buffer);
    if (FAILED(hr))
    {
        skip("Couldn't lock vertex buffer.\n");
        goto cleanup;
    }
    memcpy(vertex_buffer, vertices, num_vertices * vertex_size);
    hr = mesh->lpVtbl->UnlockVertexBuffer(mesh);
    if (FAILED(hr))
    {
        skip("Couldn't unlock vertex buffer.\n");
        goto cleanup;
    }

    hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, &index_buffer);
    if (FAILED(hr))
    {
        skip("Couldn't lock index buffer.\n");
        goto cleanup;
    }
    if (options & D3DXMESH_32BIT)
    {
        if (indices)
            memcpy(index_buffer, indices, 3 * num_faces * sizeof(DWORD));
        else
        {
            /* Fill index buffer with 0, 1, 2, ...*/
            DWORD *indices_32bit = (DWORD*)index_buffer;
            UINT i;
            for (i = 0; i < 3 * num_faces; i++)
                indices_32bit[i] = i;
        }
    }
    else
    {
        if (indices)
            memcpy(index_buffer, indices, 3 * num_faces * sizeof(WORD));
        else
        {
            /* Fill index buffer with 0, 1, 2, ...*/
            WORD *indices_16bit = (WORD*)index_buffer;
            UINT i;
            for (i = 0; i < 3 * num_faces; i++)
                indices_16bit[i] = i;
        }
    }
    hr = mesh->lpVtbl->UnlockIndexBuffer(mesh);
    if (FAILED(hr)) {
        skip("Couldn't unlock index buffer.\n");
        goto cleanup;
    }

    hr = mesh->lpVtbl->LockAttributeBuffer(mesh, 0, &attributes_buffer);
    if (FAILED(hr))
    {
        skip("Couldn't lock attributes buffer.\n");
        goto cleanup;
    }

    if (attributes)
        memcpy(attributes_buffer, attributes, num_faces * sizeof(*attributes));
    else
        memset(attributes_buffer, 0, num_faces * sizeof(*attributes));

    hr = mesh->lpVtbl->UnlockAttributeBuffer(mesh);
    if (FAILED(hr))
    {
        skip("Couldn't unlock attributes buffer.\n");
        goto cleanup;
    }

    hr = D3D_OK;
cleanup:
    return hr;
}

/* Using structs instead of bit-fields in order to avoid compiler issues. */
struct udec3
{
    UINT x;
    UINT y;
    UINT z;
    UINT w;
};

struct dec3n
{
    INT x;
    INT y;
    INT z;
    INT w;
};

static DWORD init_udec3_dword(UINT x, UINT y, UINT z, UINT w)
{
    DWORD d = 0;

    d |= x & 0x3ff;
    d |= (y << 10) & 0xffc00;
    d |= (z << 20) & 0x3ff00000;
    d |= (w << 30) & 0xc0000000;

    return d;
}

static DWORD init_dec3n_dword(INT x, INT y, INT z, INT w)
{
    DWORD d = 0;

    d |= x & 0x3ff;
    d |= (y << 10) & 0xffc00;
    d |= (z << 20) & 0x3ff00000;
    d |= (w << 30) & 0xc0000000;

    return d;
}

static struct udec3 dword_to_udec3(DWORD d)
{
    struct udec3 v;

    v.x = d & 0x3ff;
    v.y = (d & 0xffc00) >> 10;
    v.z = (d & 0x3ff00000) >> 20;
    v.w = (d & 0xc0000000) >> 30;

    return v;
}

static struct dec3n dword_to_dec3n(DWORD d)
{
    struct dec3n v;

    v.x = d & 0x3ff;
    v.y = (d & 0xffc00) >> 10;
    v.z = (d & 0x3ff00000) >> 20;
    v.w = (d & 0xc0000000) >> 30;

    return v;
}

static void check_vertex_components(int line, int mesh_number, int vertex_number, BYTE *got_ptr, const BYTE *exp_ptr, D3DVERTEXELEMENT9 *declaration)
{
    const char *usage_strings[] =
    {
        "position",
        "blend weight",
        "blend indices",
        "normal",
        "point size",
        "texture coordinates",
        "tangent",
        "binormal",
        "tessellation factor",
        "position transformed",
        "color",
        "fog",
        "depth",
        "sample"
    };
    D3DVERTEXELEMENT9 *decl_ptr;
    const float PRECISION = 1e-5f;

    for (decl_ptr = declaration; decl_ptr->Stream != 0xFF; decl_ptr++)
    {
        switch (decl_ptr->Type)
        {
            case D3DDECLTYPE_FLOAT1:
            {
                FLOAT *got = (FLOAT*)(got_ptr + decl_ptr->Offset);
                FLOAT *exp = (FLOAT*)(exp_ptr + decl_ptr->Offset);
                FLOAT diff = fabsf(*got - *exp);
                ok_(__FILE__,line)(diff <= PRECISION, "Mesh %d: Got %f for vertex %d %s, expected %f.\n",
                    mesh_number, *got, vertex_number, usage_strings[decl_ptr->Usage], *exp);
                break;
            }
            case D3DDECLTYPE_FLOAT2:
            {
                D3DXVECTOR2 *got = (D3DXVECTOR2*)(got_ptr + decl_ptr->Offset);
                D3DXVECTOR2 *exp = (D3DXVECTOR2*)(exp_ptr + decl_ptr->Offset);
                FLOAT diff = max(fabsf(got->x - exp->x), fabsf(got->y - exp->y));
                ok_(__FILE__,line)(diff <= PRECISION, "Mesh %d: Got (%f, %f) for vertex %d %s, expected (%f, %f).\n",
                    mesh_number, got->x, got->y, vertex_number, usage_strings[decl_ptr->Usage], exp->x, exp->y);
                break;
            }
            case D3DDECLTYPE_FLOAT3:
            {
                D3DXVECTOR3 *got = (D3DXVECTOR3*)(got_ptr + decl_ptr->Offset);
                D3DXVECTOR3 *exp = (D3DXVECTOR3*)(exp_ptr + decl_ptr->Offset);
                FLOAT diff = max(fabsf(got->x - exp->x), fabsf(got->y - exp->y));
                diff = max(diff, fabsf(got->z - exp->z));
                ok_(__FILE__,line)(diff <= PRECISION, "Mesh %d: Got (%f, %f, %f) for vertex %d %s, expected (%f, %f, %f).\n",
                    mesh_number, got->x, got->y, got->z, vertex_number, usage_strings[decl_ptr->Usage], exp->x, exp->y, exp->z);
                break;
            }
            case D3DDECLTYPE_FLOAT4:
            {
                D3DXVECTOR4 *got = (D3DXVECTOR4*)(got_ptr + decl_ptr->Offset);
                D3DXVECTOR4 *exp = (D3DXVECTOR4*)(exp_ptr + decl_ptr->Offset);
                FLOAT diff = max(fabsf(got->x - exp->x), fabsf(got->y - exp->y));
                diff = max(diff, fabsf(got->z - exp->z));
                diff = max(diff, fabsf(got->w - exp->w));
                ok_(__FILE__,line)(diff <= PRECISION, "Mesh %d: Got (%f, %f, %f, %f) for vertex %d %s, expected (%f, %f, %f, %f).\n",
                    mesh_number, got->x, got->y, got->z, got->w, vertex_number, usage_strings[decl_ptr->Usage], exp->x, exp->y, exp->z, got->w);
                break;
            }
            case D3DDECLTYPE_D3DCOLOR:
            {
                BYTE *got = got_ptr + decl_ptr->Offset;
                const BYTE *exp = exp_ptr + decl_ptr->Offset;
                BOOL same_color = got[0] == exp[0] && got[1] == exp[1]
                                  && got[2] == exp[2] && got[3] == exp[3];
                const char *color_types[] = {"diffuse", "specular", "undefined color"};
                BYTE usage_index = decl_ptr->UsageIndex;
                if (usage_index > 1) usage_index = 2;
                ok_(__FILE__,line)(same_color, "Mesh %d: Got (%u, %u, %u, %u) for vertex %d %s, expected (%u, %u, %u, %u).\n",
                    mesh_number, got[0], got[1], got[2], got[3], vertex_number, color_types[usage_index], exp[0], exp[1], exp[2], exp[3]);
                break;
            }
            case D3DDECLTYPE_UBYTE4:
            case D3DDECLTYPE_UBYTE4N:
            {
                BYTE *got = got_ptr + decl_ptr->Offset;
                const BYTE *exp = exp_ptr + decl_ptr->Offset;
                BOOL same = got[0] == exp[0] && got[1] == exp[1]
                            && got[2] == exp[2] && got[3] == exp[3];
                ok_(__FILE__,line)(same, "Mesh %d: Got (%u, %u, %u, %u) for vertex %d %s, expected (%u, %u, %u, %u).\n",
                    mesh_number, got[0], got[1], got[2], got[3], vertex_number, usage_strings[decl_ptr->Usage], exp[0], exp[1], exp[2], exp[3]);
                break;
            }
            case D3DDECLTYPE_SHORT2:
            case D3DDECLTYPE_SHORT2N:
            {
                SHORT *got = (SHORT*)(got_ptr + decl_ptr->Offset);
                SHORT *exp = (SHORT*)(exp_ptr + decl_ptr->Offset);
                BOOL same = got[0] == exp[0] && got[1] == exp[1];
                ok_(__FILE__,line)(same, "Mesh %d: Got (%hd, %hd) for vertex %d %s, expected (%hd, %hd).\n",
                    mesh_number, got[0], got[1], vertex_number, usage_strings[decl_ptr->Usage], exp[0], exp[1]);
                break;
            }
            case D3DDECLTYPE_SHORT4:
            case D3DDECLTYPE_SHORT4N:
            {
                SHORT *got = (SHORT*)(got_ptr + decl_ptr->Offset);
                SHORT *exp = (SHORT*)(exp_ptr + decl_ptr->Offset);
                BOOL same = got[0] == exp[0] && got[1] == exp[1]
                            && got[2] == exp[2] && got[3] == exp[3];
                ok_(__FILE__,line)(same, "Mesh %d: Got (%hd, %hd, %hd, %hd) for vertex %d %s, expected (%hd, %hd, %hd, %hd).\n",
                    mesh_number, got[0], got[1], got[2], got[3], vertex_number, usage_strings[decl_ptr->Usage], exp[0], exp[1], exp[2], exp[3]);
                break;
            }
            case D3DDECLTYPE_USHORT2N:
            {
                USHORT *got = (USHORT*)(got_ptr + decl_ptr->Offset);
                USHORT *exp = (USHORT*)(exp_ptr + decl_ptr->Offset);
                BOOL same = got[0] == exp[0] && got[1] == exp[1];
                ok_(__FILE__,line)(same, "Mesh %d: Got (%hu, %hu) for vertex %d %s, expected (%hu, %hu).\n",
                    mesh_number, got[0], got[1], vertex_number, usage_strings[decl_ptr->Usage], exp[0], exp[1]);
                break;
            }
            case D3DDECLTYPE_USHORT4N:
            {
                USHORT *got = (USHORT*)(got_ptr + decl_ptr->Offset);
                USHORT *exp = (USHORT*)(exp_ptr + decl_ptr->Offset);
                BOOL same = got[0] == exp[0] && got[1] == exp[1]
                            && got[2] == exp[2] && got[3] == exp[3];
                ok_(__FILE__,line)(same, "Mesh %d: Got (%hu, %hu, %hu, %hu) for vertex %d %s, expected (%hu, %hu, %hu, %hu).\n",
                    mesh_number, got[0], got[1], got[2], got[3], vertex_number, usage_strings[decl_ptr->Usage], exp[0], exp[1], exp[2], exp[3]);
                break;
            }
            case D3DDECLTYPE_UDEC3:
            {
                DWORD *got = (DWORD*)(got_ptr + decl_ptr->Offset);
                DWORD *exp = (DWORD*)(exp_ptr + decl_ptr->Offset);
                BOOL same = memcmp(got, exp, sizeof(*got)) == 0;
                struct udec3 got_udec3 = dword_to_udec3(*got);
                struct udec3 exp_udec3 = dword_to_udec3(*exp);
                ok_(__FILE__,line)(same, "Mesh %d: Got (%u, %u, %u, %u) for vertex %d %s, expected (%u, %u, %u, %u).\n",
                    mesh_number, got_udec3.x, got_udec3.y, got_udec3.z, got_udec3.w, vertex_number, usage_strings[decl_ptr->Usage], exp_udec3.x, exp_udec3.y, exp_udec3.z, exp_udec3.w);

                break;
            }
            case D3DDECLTYPE_DEC3N:
            {
                DWORD *got = (DWORD*)(got_ptr + decl_ptr->Offset);
                DWORD *exp = (DWORD*)(exp_ptr + decl_ptr->Offset);
                BOOL same = memcmp(got, exp, sizeof(*got)) == 0;
                struct dec3n got_dec3n = dword_to_dec3n(*got);
                struct dec3n exp_dec3n = dword_to_dec3n(*exp);
                ok_(__FILE__,line)(same, "Mesh %d: Got (%d, %d, %d, %d) for vertex %d %s, expected (%d, %d, %d, %d).\n",
                    mesh_number, got_dec3n.x, got_dec3n.y, got_dec3n.z, got_dec3n.w, vertex_number, usage_strings[decl_ptr->Usage], exp_dec3n.x, exp_dec3n.y, exp_dec3n.z, exp_dec3n.w);
                break;
            }
            case D3DDECLTYPE_FLOAT16_2:
            {
                WORD *got = (WORD*)(got_ptr + decl_ptr->Offset);
                WORD *exp = (WORD*)(exp_ptr + decl_ptr->Offset);
                BOOL same = got[0] == exp[0] && got[1] == exp[1];
                ok_(__FILE__,line)(same, "Mesh %d: Got (%hx, %hx) for vertex %d %s, expected (%hx, %hx).\n",
                    mesh_number, got[0], got[1], vertex_number, usage_strings[decl_ptr->Usage], exp[0], exp[1]);
                break;
            }
            case D3DDECLTYPE_FLOAT16_4:
            {
                WORD *got = (WORD*)(got_ptr + decl_ptr->Offset);
                WORD *exp = (WORD*)(exp_ptr + decl_ptr->Offset);
                BOOL same = got[0] == exp[0] && got[1] == exp[1]
                            && got[2] == exp[2] && got[3] == exp[3];
                ok_(__FILE__,line)(same, "Mesh %d: Got (%hx, %hx, %hx, %hx) for vertex %d %s, expected (%hx, %hx, %hx, %hx).\n",
                    mesh_number, got[0], got[1], got[2], got[3], vertex_number, usage_strings[decl_ptr->Usage], exp[0], exp[1], exp[2], exp[3]);
                break;
            }
            default:
                break;
        }
    }
}

static void test_weld_vertices(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    DWORD i;
    const DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    const DWORD options_16bit = D3DXMESH_SYSTEMMEM;
    BYTE *vertices = NULL;
    DWORD *indices = NULL;
    WORD *indices_16bit = NULL;
    const UINT VERTS_PER_FACE = 3;
    const D3DXVECTOR3 up = {0.0f, 0.0f, 1.0f};
    struct vertex_normal
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
    };
    struct vertex_blendweight
    {
        D3DXVECTOR3 position;
        FLOAT blendweight;
    };
    struct vertex_texcoord
    {
        D3DXVECTOR3 position;
        D3DXVECTOR2 texcoord;
    };
    struct vertex_color
    {
        D3DXVECTOR3 position;
        DWORD color;
    };
    struct vertex_color_ubyte4
    {
        D3DXVECTOR3 position;
        BYTE color[4];
    };
    struct vertex_texcoord_short2
    {
        D3DXVECTOR3 position;
        SHORT texcoord[2];
    };
    struct vertex_texcoord_ushort2n
    {
        D3DXVECTOR3 position;
        USHORT texcoord[2];
    };
    struct vertex_normal_short4
    {
        D3DXVECTOR3 position;
        SHORT normal[4];
    };
    struct vertex_texcoord_float16_2
    {
        D3DXVECTOR3 position;
        WORD texcoord[2];
    };
    struct vertex_texcoord_float16_4
    {
        D3DXVECTOR3 position;
        WORD texcoord[4];
    };
    struct vertex_normal_udec3
    {
        D3DXVECTOR3 position;
        DWORD normal;
    };
    struct vertex_normal_dec3n
    {
        D3DXVECTOR3 position;
        DWORD normal;
    };
    UINT vertex_size_normal = sizeof(struct vertex_normal);
    UINT vertex_size_blendweight = sizeof(struct vertex_blendweight);
    UINT vertex_size_texcoord = sizeof(struct vertex_texcoord);
    UINT vertex_size_color = sizeof(struct vertex_color);
    UINT vertex_size_color_ubyte4 = sizeof(struct vertex_color_ubyte4);
    UINT vertex_size_texcoord_short2 = sizeof(struct vertex_texcoord_short2);
    UINT vertex_size_normal_short4 = sizeof(struct vertex_normal_short4);
    UINT vertex_size_texcoord_float16_2 = sizeof(struct vertex_texcoord_float16_2);
    UINT vertex_size_texcoord_float16_4 = sizeof(struct vertex_texcoord_float16_4);
    UINT vertex_size_normal_udec3 = sizeof(struct vertex_normal_udec3);
    UINT vertex_size_normal_dec3n = sizeof(struct vertex_normal_dec3n);
    D3DVERTEXELEMENT9 declaration_normal[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_normal3[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 3},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_blendweight[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_texcoord[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_color[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_color_ubyte4n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_color_ubyte4[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_texcoord_short2[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_texcoord_short2n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_texcoord_ushort2n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_USHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_normal_short4[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_normal_short4n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_normal_ushort4n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_USHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_texcoord10[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 10},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_color2[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 2},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_color1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_texcoord_float16_2[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_texcoord_float16_4[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_normal_udec3[] =
   {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_UDEC3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_normal_dec3n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_DEC3N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    /* Test 0. One face and no welding.
     *
     * 0--1
     * | /
     * |/
     * 2
     */
    const struct vertex vertices0[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},
    };
    const DWORD indices0[] = {0, 1, 2};
    const DWORD attributes0[] = {0};
    const DWORD exp_indices0[] = {0, 1, 2};
    const UINT num_vertices0 = ARRAY_SIZE(vertices0);
    const UINT num_faces0 = ARRAY_SIZE(indices0) / VERTS_PER_FACE;
    const DWORD flags0 = D3DXWELDEPSILONS_WELDALL;
    /* epsilons0 is NULL */
    const DWORD adjacency0[] = {-1, -1, -1};
    const struct vertex exp_vertices0[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},
    };
    const DWORD exp_face_remap0[] = {0};
    const DWORD exp_vertex_remap0[] = {0, 1, 2};
    const DWORD exp_new_num_vertices0 = ARRAY_SIZE(exp_vertices0);
    /* Test 1. Two vertices should be removed without regard to epsilon.
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_normal vertices1[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, up},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, up},
    };
    const DWORD indices1[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes1[] = {0, 0};
    const UINT num_vertices1 = ARRAY_SIZE(vertices1);
    const UINT num_faces1 = ARRAY_SIZE(indices1) / VERTS_PER_FACE;
    const DWORD flags1 = D3DXWELDEPSILONS_WELDALL;
    /* epsilons1 is NULL */
    const DWORD adjacency1[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices1[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  0.0f,  0.f}, up}
    };
    const DWORD exp_indices1[] = {0, 1, 2, 1, 3, 2};
    const DWORD exp_face_remap1[] = {0, 1};
    const DWORD exp_vertex_remap1[] = {0, 1, 2, 4, -1, -1};
    const DWORD exp_new_num_vertices1 = ARRAY_SIZE(exp_vertices1);
    /* Test 2. Two faces. No vertices should be removed because of normal
     * epsilon, but the positions should be replaced. */
    const struct vertex_normal vertices2[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD indices2[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes2[] = {0, 0};
    const UINT num_vertices2 = ARRAY_SIZE(vertices2);
    const UINT num_faces2 = ARRAY_SIZE(indices2) / VERTS_PER_FACE;
    DWORD flags2 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons2 = {1.0f, 0.0f, 0.499999f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency2[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices2[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 2.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD exp_indices2[] = {0, 1, 2, 3, 4, 5};
    const DWORD exp_face_remap2[] = {0, 1};
    const DWORD exp_vertex_remap2[] = {0, 1, 2, 3, 4, 5};
    const DWORD exp_new_num_vertices2 = ARRAY_SIZE(exp_vertices2);
    /* Test 3. Two faces. One vertex should be removed because of normal epsilon. */
    const struct vertex_normal vertices3[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD indices3[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes3[] = {0, 0};
    const UINT num_vertices3 = ARRAY_SIZE(vertices3);
    const UINT num_faces3 = ARRAY_SIZE(indices3) / VERTS_PER_FACE;
    DWORD flags3 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons3 = {1.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency3[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices3[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD exp_indices3[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap3[] = {0, 1};
    const DWORD exp_vertex_remap3[] = {0, 1, 2, 4, 5, -1};
    const DWORD exp_new_num_vertices3 = ARRAY_SIZE(exp_vertices3);
    /* Test 4  Two faces. Two vertices should be removed. */
    const struct vertex_normal vertices4[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD indices4[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes4[] = {0, 0};
    const UINT num_vertices4 = ARRAY_SIZE(vertices4);
    const UINT num_faces4 = ARRAY_SIZE(indices4) / VERTS_PER_FACE;
    DWORD flags4 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons4 = {1.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency4[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices4[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  0.0f,  0.f}, up},
    };
    const DWORD exp_indices4[] = {0, 1, 2, 1, 3, 2};
    const DWORD exp_face_remap4[] = {0, 1};
    const DWORD exp_vertex_remap4[] = {0, 1, 2, 4, -1, -1};
    const DWORD exp_new_num_vertices4 = ARRAY_SIZE(exp_vertices4);
    /* Test 5. Odd face ordering.
     *
     * 0--1 6 3
     * | / /| |\
     * |/ / | | \
     * 2 8--7 5--4
     */
    const struct vertex_normal vertices5[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, up},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, up},

        {{ 4.0f,  3.0f,  0.f}, up},
        {{ 6.0f,  0.0f,  0.f}, up},
        {{ 4.0f,  0.0f,  0.f}, up},
    };
    const DWORD indices5[] = {0, 1, 2, 6, 7, 8, 3, 4, 5};
    const DWORD exp_indices5[] = {0, 1, 2, 1, 4, 2, 1, 3, 4};
    const DWORD attributes5[] = {0, 0, 0};
    const UINT num_vertices5 = ARRAY_SIZE(vertices5);
    const UINT num_faces5 = ARRAY_SIZE(indices5) / VERTS_PER_FACE;
    DWORD flags5 = D3DXWELDEPSILONS_WELDALL;
    const DWORD adjacency5[] = {-1, 1, -1, 2, -1, 0, -1, -1, 1};
    const struct vertex_normal exp_vertices5[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, up},
    };
    const DWORD exp_face_remap5[] = {0, 1, 2};
    const DWORD exp_vertex_remap5[] = {0, 1, 2, 4, 5, -1, -1, -1, -1};
    const DWORD exp_new_num_vertices5 = ARRAY_SIZE(exp_vertices5);
    /* Test 6. Two faces. Do not remove flag is used, so no vertices should be
     * removed. */
    const struct vertex_normal vertices6[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD indices6[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes6[] = {0, 0};
    const UINT num_vertices6 = ARRAY_SIZE(vertices6);
    const UINT num_faces6 = ARRAY_SIZE(indices6) / VERTS_PER_FACE;
    DWORD flags6 = D3DXWELDEPSILONS_WELDPARTIALMATCHES | D3DXWELDEPSILONS_DONOTREMOVEVERTICES;
    const D3DXWELDEPSILONS epsilons6 = {1.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency6[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices6[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

    };
    const DWORD exp_indices6[] = {0, 1, 2, 3, 4, 5};
    const DWORD exp_face_remap6[] = {0, 1};
    const DWORD exp_vertex_remap6[] = {0, 1, 2, 3, 4, 5};
    const DWORD exp_new_num_vertices6 = ARRAY_SIZE(exp_vertices6);
    /* Test 7. Same as test 6 but with 16 bit indices. */
    const WORD indices6_16bit[] = {0, 1, 2, 3, 4, 5};
    /* Test 8. No flags. Same result as D3DXWELDEPSILONS_WELDPARTIALMATCHES. */
    const struct vertex_normal vertices8[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD indices8[] = {0, 1, 2, 1, 3, 4};
    const DWORD attributes8[] = {0, 0};
    const UINT num_vertices8 = ARRAY_SIZE(vertices8);
    const UINT num_faces8 = ARRAY_SIZE(indices8) / VERTS_PER_FACE;
    DWORD flags8 = 0;
    const D3DXWELDEPSILONS epsilons8 = {1.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency8[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices8[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
    };
    const DWORD exp_indices8[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap8[] = {0, 1};
    const DWORD exp_vertex_remap8[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices8 = ARRAY_SIZE(exp_vertices8);
    /* Test 9. Vertices are removed even though they belong to separate
     * attribute groups if D3DXWELDEPSILONS_DONOTSPLIT is set. */
    const struct vertex_normal vertices9[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD indices9[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes9[] = {0, 1};
    const UINT num_vertices9 = ARRAY_SIZE(vertices9);
    const UINT num_faces9 = ARRAY_SIZE(indices9) / VERTS_PER_FACE;
    DWORD flags9 = D3DXWELDEPSILONS_WELDPARTIALMATCHES | D3DXWELDEPSILONS_DONOTSPLIT;
    const D3DXWELDEPSILONS epsilons9 = {1.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency9[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices9[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  0.0f,  0.f}, up},
    };
    const DWORD exp_indices9[] = {0, 1, 2, 1, 3, 2};
    const DWORD exp_face_remap9[] = {0, 1};
    const DWORD exp_vertex_remap9[] = {0, 1, 2, 4, -1, -1};
    const DWORD exp_new_num_vertices9 = ARRAY_SIZE(exp_vertices9);
    /* Test 10. Weld blendweight (FLOAT1). */
    const struct vertex_blendweight vertices10[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 1.0f},
        {{ 2.0f,  3.0f,  0.f}, 1.0f},
        {{ 0.0f,  0.0f,  0.f}, 1.0f},

        {{ 3.0f,  3.0f,  0.f}, 0.9},
        {{ 3.0f,  0.0f,  0.f}, 1.0},
        {{ 1.0f,  0.0f,  0.f}, 0.4},
    };
    const DWORD indices10[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes10[] = {0, 0};
    const UINT num_vertices10 = ARRAY_SIZE(vertices10);
    const UINT num_faces10 = ARRAY_SIZE(indices10) / VERTS_PER_FACE;
    DWORD flags10 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons10 = {1.0f, 0.1f + FLT_EPSILON, 0.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency10[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_blendweight exp_vertices10[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 1.0f},
        {{ 2.0f,  3.0f,  0.f}, 1.0f},
        {{ 0.0f,  0.0f,  0.f}, 1.0f},

        {{ 3.0f,  0.0f,  0.f}, 1.0},
        {{ 0.0f,  0.0f,  0.f}, 0.4},
    };
    const DWORD exp_indices10[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap10[] = {0, 1};
    const DWORD exp_vertex_remap10[] = {0, 1, 2, 4, 5, -1};
    const DWORD exp_new_num_vertices10 = ARRAY_SIZE(exp_vertices10);
    /* Test 11. Weld texture coordinates. */
    const struct vertex_texcoord vertices11[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.5f, 0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, {0.2f, 0.3f}},
        {{ 3.0f,  0.0f,  0.f}, {1.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {0.1f, 0.2f}}
    };
    const DWORD indices11[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes11[] = {0, 0};
    const UINT num_vertices11 = ARRAY_SIZE(vertices11);
    const UINT num_faces11 = ARRAY_SIZE(indices11) / VERTS_PER_FACE;
    DWORD flags11 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons11 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {0.4f + FLT_EPSILON, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency11[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_texcoord exp_vertices11[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.5f, 0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  0.0f,  0.f}, {1.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {0.1f, 0.2f}},
    };
    const DWORD exp_indices11[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap11[] = {0, 1};
    const DWORD exp_vertex_remap11[] = {0, 1, 2, 4, 5, -1};
    const DWORD exp_new_num_vertices11 = ARRAY_SIZE(exp_vertices11);
    /* Test 12. Weld with color. */
    const struct vertex_color vertices12[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 2.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 0.0f,  0.0f,  0.f}, 0xFFFFFFFF},

        {{ 3.0f,  3.0f,  0.f}, 0x00000000},
        {{ 3.0f,  0.0f,  0.f}, 0xFFFFFFFF},
        {{ 1.0f,  0.0f,  0.f}, 0x88888888},
    };
    const DWORD indices12[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes12[] = {0, 0};
    const UINT num_vertices12 = ARRAY_SIZE(vertices12);
    const UINT num_faces12 = ARRAY_SIZE(indices12) / VERTS_PER_FACE;
    DWORD flags12 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons12 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency12[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_color exp_vertices12[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 2.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 0.0f,  0.0f,  0.f}, 0xFFFFFFFF},

        {{ 2.0f,  3.0f,  0.f}, 0x00000000},
        {{ 3.0f,  0.0f,  0.f}, 0xFFFFFFFF},
    };
    const DWORD exp_indices12[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap12[] = {0, 1};
    const DWORD exp_vertex_remap12[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices12 = ARRAY_SIZE(exp_vertices12);
    /* Test 13. Two faces. One vertex should be removed because of normal epsilon.
     * This is similar to test 3, but the declaration has been changed to NORMAL3.
     */
    const struct vertex_normal vertices13[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 0.5f, 0.5f}},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD indices13[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes13[] = {0, 0};
    const UINT num_vertices13 = ARRAY_SIZE(vertices3);
    const UINT num_faces13 = ARRAY_SIZE(indices3) / VERTS_PER_FACE;
    DWORD flags13 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons13 = {1.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency13[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal exp_vertices13[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, {0.2f, 0.4f, 0.4f}},
    };
    const DWORD exp_indices13[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap13[] = {0, 1};
    const DWORD exp_vertex_remap13[] = {0, 1, 2, 4, 5, -1};
    const DWORD exp_new_num_vertices13 = ARRAY_SIZE(exp_vertices13);
    /* Test 14. Another test for welding with color. */
    const struct vertex_color vertices14[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 2.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 0.0f,  0.0f,  0.f}, 0xFFFFFFFF},

        {{ 3.0f,  3.0f,  0.f}, 0x00000000},
        {{ 3.0f,  0.0f,  0.f}, 0xFFFFFFFF},
        {{ 1.0f,  0.0f,  0.f}, 0x01010101},
    };
    const DWORD indices14[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes14[] = {0, 0};
    const UINT num_vertices14 = ARRAY_SIZE(vertices14);
    const UINT num_faces14 = ARRAY_SIZE(indices14) / VERTS_PER_FACE;
    DWORD flags14 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons14 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 254.0f/255.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency14[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_color exp_vertices14[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 2.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 0.0f,  0.0f,  0.f}, 0xFFFFFFFF},

        {{ 2.0f,  3.0f,  0.f}, 0x00000000},
        {{ 3.0f,  0.0f,  0.f}, 0xFFFFFFFF},
    };
    const DWORD exp_indices14[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap14[] = {0, 1};
    const DWORD exp_vertex_remap14[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices14 = ARRAY_SIZE(exp_vertices14);
    /* Test 15. Weld with color, but as UBYTE4N instead of D3DCOLOR. It shows
     * that UBYTE4N and D3DCOLOR are compared the same way.
     */
    const struct vertex_color_ubyte4 vertices15[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 2.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 0.0f,  0.0f,  0.f}, {255, 255, 255, 255}},

        {{ 3.0f,  3.0f,  0.f}, {  0,   0,   0,   0}},
        {{ 3.0f,  0.0f,  0.f}, {255, 255, 255, 255}},
        {{ 1.0f,  0.0f,  0.f}, {  1,   1,   1,   1}},
    };
    const DWORD indices15[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes15[] = {0, 0};
    const UINT num_vertices15 = ARRAY_SIZE(vertices15);
    const UINT num_faces15 = ARRAY_SIZE(indices15) / VERTS_PER_FACE;
    DWORD flags15 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons15 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 254.0f/255.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency15[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_color_ubyte4 exp_vertices15[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 2.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 0.0f,  0.0f,  0.f}, {255, 255, 255, 255}},

        {{ 2.0f,  3.0f,  0.f}, {  0,   0,   0,   0}},
        {{ 3.0f,  0.0f,  0.f}, {255, 255, 255, 255}},
    };
    const DWORD exp_indices15[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap15[] = {0, 1};
    const DWORD exp_vertex_remap15[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices15 = ARRAY_SIZE(exp_vertices15);
    /* Test 16. Weld with color, but as UBYTE4 instead of D3DCOLOR. It shows
     * that UBYTE4 is not normalized and that epsilon is truncated and compared
     * directly to each of the four bytes.
     */
    const struct vertex_color_ubyte4 vertices16[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 2.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 0.0f,  0.0f,  0.f}, {255, 255, 255, 255}},

        {{ 3.0f,  3.0f,  0.f}, {  0,   0,   0,   0}},
        {{ 3.0f,  0.0f,  0.f}, {255, 255, 255, 255}},
        {{ 1.0f,  0.0f,  0.f}, {  1,   1,   1,   1}},
    };
    const DWORD indices16[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes16[] = {0, 0};
    const UINT num_vertices16 = ARRAY_SIZE(vertices16);
    const UINT num_faces16 = ARRAY_SIZE(indices16) / VERTS_PER_FACE;
    DWORD flags16 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons16 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 254.9f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency16[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_color_ubyte4 exp_vertices16[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 2.0f,  3.0f,  0.f}, {255, 255, 255, 255}},
        {{ 0.0f,  0.0f,  0.f}, {255, 255, 255, 255}},

        {{ 2.0f,  3.0f,  0.f}, {  0,   0,   0,   0}},
        {{ 3.0f,  0.0f,  0.f}, {255, 255, 255, 255}},
    };
    const DWORD exp_indices16[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap16[] = {0, 1};
    const DWORD exp_vertex_remap16[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices16 = ARRAY_SIZE(exp_vertices16);
    /* Test 17. Weld texture coordinates but as SHORT2 instead of D3DXVECTOR2.*/
    const struct vertex_texcoord_short2 vertices17[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, { 0, 0}},

        {{ 3.0f,  3.0f,  0.f}, {32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0}},
        {{ 1.0f,  0.0f,  0.f}, {32766, 32766}},
    };
    const DWORD indices17[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes17[] = {0, 0};
    const UINT num_vertices17 = ARRAY_SIZE(vertices17);
    const UINT num_faces17 = ARRAY_SIZE(indices17) / VERTS_PER_FACE;
    DWORD flags17 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons17 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {32766.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency17[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_texcoord_short2 exp_vertices17[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, { 0, 0}},

        {{ 2.0f,  3.0f,  0.f}, {32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0}},
    };
    const DWORD exp_indices17[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap17[] = {0, 1};
    const DWORD exp_vertex_remap17[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices17 = ARRAY_SIZE(exp_vertices17);
    /* Test 18. Weld texture coordinates but as SHORT2N instead of D3DXVECTOR2. */
    const struct vertex_texcoord_short2 vertices18[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, { 0, 0}},

        {{ 3.0f,  3.0f,  0.f}, {32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0}},
        {{ 1.0f,  0.0f,  0.f}, {32766, 32766}},
    };
    const DWORD indices18[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes18[] = {0, 0};
    const UINT num_vertices18 = ARRAY_SIZE(vertices18);
    const UINT num_faces18 = ARRAY_SIZE(indices18) / VERTS_PER_FACE;
    DWORD flags18 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons18 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {32766.0f/32767.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency18[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_texcoord_short2 exp_vertices18[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, { 0, 0}},

        {{ 2.0f,  3.0f,  0.f}, {32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0}},
    };
    const DWORD exp_indices18[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap18[] = {0, 1};
    const DWORD exp_vertex_remap18[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices18 = ARRAY_SIZE(exp_vertices18);
    /* Test 19.  Weld texture coordinates but as USHORT2N instead of D3DXVECTOR2. */
    const struct vertex_texcoord_ushort2n vertices19[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, { 0, 0}},

        {{ 3.0f,  3.0f,  0.f}, {65535, 65535}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0}},
        {{ 1.0f,  0.0f,  0.f}, {65534, 65534}},
    };
    const DWORD indices19[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes19[] = {0, 0};
    const UINT num_vertices19 = ARRAY_SIZE(vertices19);
    const UINT num_faces19 = ARRAY_SIZE(indices19) / VERTS_PER_FACE;
    DWORD flags19 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons19 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {65534.0f/65535.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency19[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_texcoord_ushort2n exp_vertices19[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, { 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, { 0, 0}},

        {{ 2.0f,  3.0f,  0.f}, {65535, 65535}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0}},
    };
    const DWORD exp_indices19[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap19[] = {0, 1};
    const DWORD exp_vertex_remap19[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices19 = ARRAY_SIZE(exp_vertices19);
    /* Test 20.  Weld normal as SHORT4 instead of D3DXVECTOR3. */
    const struct vertex_normal_short4 vertices20[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 0}},

        {{ 3.0f,  3.0f,  0.f}, {32767, 32767, 32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 0, 0}},
        {{ 1.0f,  0.0f,  0.f}, {32766, 32766, 32766, 32766}},
    };
    const DWORD indices20[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes20[] = {0, 0};
    const UINT num_vertices20 = ARRAY_SIZE(vertices20);
    const UINT num_faces20 = ARRAY_SIZE(indices20) / VERTS_PER_FACE;
    DWORD flags20 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons20 = {1.0f, 0.0f, 32766.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency20[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal_short4 exp_vertices20[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 0}},

        {{ 2.0f,  3.0f,  0.f}, {32767, 32767, 32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 0, 0}},
    };
    const DWORD exp_indices20[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap20[] = {0, 1};
    const DWORD exp_vertex_remap20[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices20 = ARRAY_SIZE(exp_vertices20);
    /* Test 21.  Weld normal as SHORT4N instead of D3DXVECTOR3. */
    const struct vertex_normal_short4 vertices21[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 0}},

        {{ 3.0f,  3.0f,  0.f}, {32767, 32767, 32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 0, 0}},
        {{ 1.0f,  0.0f,  0.f}, {32766, 32766, 32766, 32766}},
    };
    const DWORD indices21[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes21[] = {0, 0};
    const UINT num_vertices21 = ARRAY_SIZE(vertices21);
    const UINT num_faces21 = ARRAY_SIZE(indices21) / VERTS_PER_FACE;
    DWORD flags21 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons21 = {1.0f, 0.0f, 32766.0f/32767.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency21[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal_short4 exp_vertices21[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 0}},

        {{ 2.0f,  3.0f,  0.f}, {32767, 32767, 32767, 32767}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 0, 0}},
    };
    const DWORD exp_indices21[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap21[] = {0, 1};
    const DWORD exp_vertex_remap21[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices21 = ARRAY_SIZE(exp_vertices21);
    /* Test 22.  Weld normal as USHORT4N instead of D3DXVECTOR3. */
    const struct vertex_normal_short4 vertices22[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 0}},

        {{ 3.0f,  3.0f,  0.f}, {-1, -1, -1, -1}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 0, 0}},
        {{ 1.0f,  0.0f,  0.f}, {-2, -2, -2, -2}},
    };
    const DWORD indices22[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes22[] = {0, 0};
    const UINT num_vertices22 = ARRAY_SIZE(vertices22);
    const UINT num_faces22 = ARRAY_SIZE(indices22) / VERTS_PER_FACE;
    DWORD flags22 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons22 = {1.0f, 0.0f, 65534.0f/65535.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency22[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal_short4 exp_vertices22[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 0}},

        {{ 2.0f,  3.0f,  0.f}, {-1, -1, -1, -1}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 0, 0}},
    };
    const DWORD exp_indices22[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap22[] = {0, 1};
    const DWORD exp_vertex_remap22[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices22 = ARRAY_SIZE(exp_vertices22);
    /* Test 23. Weld texture coordinates as FLOAT16_2. Similar to test 11, but
     * with texture coordinates converted to float16 in hex. */
    const struct vertex_texcoord_float16_2 vertices23[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a}}, /* {0.5f, 0.7f} */
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd}}, /* {-0.2f, -0.3f} */

        {{ 3.0f,  3.0f,  0.f}, {0x3266, 0x34cd}}, /* {0.2f, 0.3f} */
        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 1.0f,  0.0f,  0.f}, {0x2e66, 0x3266}}, /* {0.1f, 0.2f} */
    };
    const DWORD indices23[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes23[] = {0, 0};
    const UINT num_vertices23 = ARRAY_SIZE(vertices23);
    const UINT num_faces23 = ARRAY_SIZE(indices23) / VERTS_PER_FACE;
    DWORD flags23 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons23 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {0.41f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency23[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_texcoord_float16_2 exp_vertices23[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a}}, /* {0.5f, 0.7f} */
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd}}, /* {-0.2f, -0.3f} */

        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 0.0f,  0.0f,  0.f}, {0x2e66, 0x3266}}, /* {0.1f, 0.2f} */
    };
    const DWORD exp_indices23[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap23[] = {0, 1};
    const DWORD exp_vertex_remap23[] = {0, 1, 2, 4, 5, -1};
    const DWORD exp_new_num_vertices23 = ARRAY_SIZE(exp_vertices23);
    /* Test 24. Weld texture coordinates as FLOAT16_4. Similar to test 24. */
    const struct vertex_texcoord_float16_4 vertices24[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00, 0x3c00, 0x3c00}},
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a, 0x3800, 0x399a}},
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd, 0xb266, 0xb4cd}},

        {{ 3.0f,  3.0f,  0.f}, {0x3266, 0x34cd, 0x3266, 0x34cd}},
        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00, 0x3c00, 0x3c00}},
        {{ 1.0f,  0.0f,  0.f}, {0x2e66, 0x3266, 0x2e66, 0x3266}},
    };
    const DWORD indices24[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes24[] = {0, 0};
    const UINT num_vertices24 = ARRAY_SIZE(vertices24);
    const UINT num_faces24 = ARRAY_SIZE(indices24) / VERTS_PER_FACE;
    DWORD flags24 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons24 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {0.41f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency24[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_texcoord_float16_4 exp_vertices24[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00, 0x3c00, 0x3c00}},
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a, 0x3800, 0x399a}},
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd, 0xb266, 0xb4cd}},

        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00, 0x3c00, 0x3c00}},
        {{ 0.0f,  0.0f,  0.f}, {0x2e66, 0x3266, 0x2e66, 0x3266}},
    };
    const DWORD exp_indices24[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap24[] = {0, 1};
    const DWORD exp_vertex_remap24[] = {0, 1, 2, 4, 5, -1};
    const DWORD exp_new_num_vertices24 = ARRAY_SIZE(exp_vertices24);
    /* Test 25. Weld texture coordinates with usage index 10 (TEXCOORD10). The
     * usage index is capped at 7, so the epsilon for TEXCOORD7 is used instead.
     */
    const struct vertex_texcoord vertices25[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.5f, 0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, {0.2f, 0.3f}},
        {{ 3.0f,  0.0f,  0.f}, {1.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {0.1f, 0.2f}}
    };
    const DWORD indices25[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes25[] = {0, 0};
    const UINT num_vertices25 = ARRAY_SIZE(vertices25);
    const UINT num_faces25 = ARRAY_SIZE(indices25) / VERTS_PER_FACE;
    DWORD flags25 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons25 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f + FLT_EPSILON}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency25[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_texcoord exp_vertices25[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.5f, 0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  0.0f,  0.f}, {1.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {0.1f, 0.2f}},
    };
    const DWORD exp_indices25[] = {0, 1, 2, 1, 3, 4};
    const DWORD exp_face_remap25[] = {0, 1};
    const DWORD exp_vertex_remap25[] = {0, 1, 2, 4, 5, -1};
    const DWORD exp_new_num_vertices25 = ARRAY_SIZE(exp_vertices25);
    /* Test 26. Weld color with usage index larger than 1. Shows that none of
     * the epsilon values are used. */
    const struct vertex_color vertices26[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 2.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 0.0f,  0.0f,  0.f}, 0xFFFFFFFF},

        {{ 3.0f,  3.0f,  0.f}, 0x00000000},
        {{ 3.0f,  0.0f,  0.f}, 0xFFFFFFFF},
        {{ 1.0f,  0.0f,  0.f}, 0x01010101},
    };
    const DWORD indices26[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes26[] = {0, 0};
    const UINT num_vertices26 = ARRAY_SIZE(vertices26);
    const UINT num_faces26 = ARRAY_SIZE(indices26) / VERTS_PER_FACE;
    DWORD flags26 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons26 = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 1.0f, 1.0f};
    const DWORD adjacency26[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_color exp_vertices26[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 2.0f,  3.0f,  0.f}, 0xFFFFFFFF},
        {{ 0.0f,  0.0f,  0.f}, 0xFFFFFFFF},

        {{ 2.0f,  3.0f,  0.f}, 0x00000000},
        {{ 3.0f,  0.0f,  0.f}, 0xFFFFFFFF},
        {{ 0.0f,  0.0f,  0.f}, 0x01010101},
    };
    const DWORD exp_indices26[] = {0, 1, 2, 3, 4, 5};
    const DWORD exp_face_remap26[] = {0, 1};
    const DWORD exp_vertex_remap26[] = {0, 1, 2, 3, 4, 5};
    const DWORD exp_new_num_vertices26 = ARRAY_SIZE(exp_vertices26);
    /* Test 27. Weld color with usage index 1 (specular). */
    /* Previously this test used float color values and index > 1 but that case
     * appears to be effectively unhandled in native so the test gave
     * inconsistent results. */
    const struct vertex_color vertices27[] =
    {
        {{ 0.0f,  3.0f,  0.0f}, 0x00000000},
        {{ 2.0f,  3.0f,  0.0f}, 0x10203040},
        {{ 0.0f,  0.0f,  0.0f}, 0x50607080},

        {{ 3.0f,  3.0f,  0.0f}, 0x11213141},
        {{ 3.0f,  0.0f,  0.0f}, 0xffffffff},
        {{ 1.0f,  0.0f,  0.0f}, 0x51617181},
    };
    const DWORD indices27[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes27[] = {0, 0};
    const UINT num_vertices27 = ARRAY_SIZE(vertices27);
    const UINT num_faces27 = ARRAY_SIZE(indices27) / VERTS_PER_FACE;
    DWORD flags27 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons27 =
    {
        1.1f, 0.0f, 0.0f, 0.0f, 2.0f / 255.0f, 0.0f,
        {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f
    };
    const DWORD adjacency27[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_color exp_vertices27[] =
    {
        {{ 0.0f,  3.0f,  0.0f}, 0x00000000},
        {{ 2.0f,  3.0f,  0.0f}, 0x10203040},
        {{ 0.0f,  0.0f,  0.0f}, 0x50607080},

        {{ 3.0f,  0.0f,  0.0f}, 0xffffffff},
    };
    const DWORD exp_indices27[] = {0, 1, 2, 1, 3, 2};
    const DWORD exp_face_remap27[] = {0, 1};
    const DWORD exp_vertex_remap27[] = {0, 1, 2, 4, -1, -1};
    const DWORD exp_new_num_vertices27 = ARRAY_SIZE(exp_vertices27);
    /* Test 28. Weld one normal with UDEC3. */
    const DWORD dword_udec3_zero = init_udec3_dword(0, 0, 0, 1);
    const DWORD dword_udec3_1023 = init_udec3_dword(1023, 1023, 1023, 1);
    const DWORD dword_udec3_1022 = init_udec3_dword(1022, 1022, 1022, 1);
    const struct vertex_normal_udec3 vertices28[] =
    {
        {{ 0.0f,  3.0f,  0.f}, dword_udec3_zero},
        {{ 2.0f,  3.0f,  0.f}, dword_udec3_zero},
        {{ 0.0f,  0.0f,  0.f}, dword_udec3_zero},

        {{ 3.0f,  3.0f,  0.f}, dword_udec3_1023},
        {{ 3.0f,  0.0f,  0.f}, dword_udec3_zero},
        {{ 1.0f,  0.0f,  0.f}, dword_udec3_1022},
    };
    const DWORD indices28[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes28[] = {0, 0};
    const UINT num_vertices28 = ARRAY_SIZE(vertices28);
    const UINT num_faces28 = ARRAY_SIZE(indices28) / VERTS_PER_FACE;
    DWORD flags28 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons28 = {1.0f, 0.0f, 1022.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency28[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal_udec3 exp_vertices28[] =
    {
        {{ 0.0f,  3.0f,  0.f}, dword_udec3_zero},
        {{ 2.0f,  3.0f,  0.f}, dword_udec3_zero},
        {{ 0.0f,  0.0f,  0.f}, dword_udec3_zero},

        {{ 2.0f,  3.0f,  0.f}, dword_udec3_1023},
        {{ 3.0f,  0.0f,  0.f}, dword_udec3_zero},
    };
    const DWORD exp_indices28[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap28[] = {0, 1};
    const DWORD exp_vertex_remap28[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices28 = ARRAY_SIZE(exp_vertices28);
    /* Test 29. Weld one normal with DEC3N. */
    const DWORD dword_dec3n_zero = init_dec3n_dword(0, 0, 0, 1);
    const DWORD dword_dec3n_511 = init_dec3n_dword(511, 511, 511, 1);
    const DWORD dword_dec3n_510 = init_dec3n_dword(510, 510, 510, 1);
    const struct vertex_normal_dec3n vertices29[] =
    {
        {{ 0.0f,  3.0f,  0.f}, dword_dec3n_zero},
        {{ 2.0f,  3.0f,  0.f}, dword_dec3n_zero},
        {{ 0.0f,  0.0f,  0.f}, dword_dec3n_zero},

        {{ 3.0f,  3.0f,  0.f}, dword_dec3n_511},
        {{ 3.0f,  0.0f,  0.f}, dword_dec3n_zero},
        {{ 1.0f,  0.0f,  0.f}, dword_dec3n_510},
    };
    const DWORD indices29[] = {0, 1, 2, 3, 4, 5};
    const DWORD attributes29[] = {0, 0};
    const UINT num_vertices29 = ARRAY_SIZE(vertices29);
    const UINT num_faces29 = ARRAY_SIZE(indices29) / VERTS_PER_FACE;
    DWORD flags29 = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    const D3DXWELDEPSILONS epsilons29 = {1.0f, 0.0f, 510.0f/511.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, .0f}, 0.0f, 0.0f, 0.0f};
    const DWORD adjacency29[] = {-1, 1, -1, -1, -1, 0};
    const struct vertex_normal_dec3n exp_vertices29[] =
    {
        {{ 0.0f,  3.0f,  0.f}, dword_dec3n_zero},
        {{ 2.0f,  3.0f,  0.f}, dword_dec3n_zero},
        {{ 0.0f,  0.0f,  0.f}, dword_dec3n_zero},

        {{ 2.0f,  3.0f,  0.f}, dword_dec3n_511},
        {{ 3.0f,  0.0f,  0.f}, dword_dec3n_zero},
    };
    const DWORD exp_indices29[] = {0, 1, 2, 3, 4, 2};
    const DWORD exp_face_remap29[] = {0, 1};
    const DWORD exp_vertex_remap29[] = {0, 1, 2, 3, 4, -1};
    const DWORD exp_new_num_vertices29 = ARRAY_SIZE(exp_vertices29);
    /* All mesh data */
    DWORD *adjacency_out = NULL;
    DWORD *face_remap = NULL;
    ID3DXMesh *mesh = NULL;
    ID3DXBuffer *vertex_remap = NULL;
    struct
    {
        const BYTE *vertices;
        const DWORD *indices;
        const DWORD *attributes;
        const DWORD num_vertices;
        const DWORD num_faces;
        const DWORD options;
        D3DVERTEXELEMENT9 *declaration;
        const UINT vertex_size;
        const DWORD flags;
        const D3DXWELDEPSILONS *epsilons;
        const DWORD *adjacency;
        const BYTE *exp_vertices;
        const DWORD *exp_indices;
        const DWORD *exp_face_remap;
        const DWORD *exp_vertex_remap;
        const DWORD exp_new_num_vertices;
    }
    tc[] =
    {
        {
            (BYTE*)vertices0,
            indices0,
            attributes0,
            num_vertices0,
            num_faces0,
            options,
            declaration_normal,
            vertex_size_normal,
            flags0,
            NULL,
            adjacency0,
            (BYTE*)exp_vertices0,
            exp_indices0,
            exp_face_remap0,
            exp_vertex_remap0,
            exp_new_num_vertices0
        },
        {
            (BYTE*)vertices1,
            indices1,
            attributes1,
            num_vertices1,
            num_faces1,
            options,
            declaration_normal,
            vertex_size_normal,
            flags1,
            NULL,
            adjacency1,
            (BYTE*)exp_vertices1,
            exp_indices1,
            exp_face_remap1,
            exp_vertex_remap1,
            exp_new_num_vertices1
        },
        {
            (BYTE*)vertices2,
            indices2,
            attributes2,
            num_vertices2,
            num_faces2,
            options,
            declaration_normal,
            vertex_size_normal,
            flags2,
            &epsilons2,
            adjacency2,
            (BYTE*)exp_vertices2,
            exp_indices2,
            exp_face_remap2,
            exp_vertex_remap2,
            exp_new_num_vertices2
        },
        {
            (BYTE*)vertices3,
            indices3,
            attributes3,
            num_vertices3,
            num_faces3,
            options,
            declaration_normal,
            vertex_size_normal,
            flags3,
            &epsilons3,
            adjacency3,
            (BYTE*)exp_vertices3,
            exp_indices3,
            exp_face_remap3,
            exp_vertex_remap3,
            exp_new_num_vertices3
        },
        {
            (BYTE*)vertices4,
            indices4,
            attributes4,
            num_vertices4,
            num_faces4,
            options,
            declaration_normal,
            vertex_size_normal,
            flags4,
            &epsilons4,
            adjacency4,
            (BYTE*)exp_vertices4,
            exp_indices4,
            exp_face_remap4,
            exp_vertex_remap4,
            exp_new_num_vertices4
        },
        /* Unusual ordering. */
        {
            (BYTE*)vertices5,
            indices5,
            attributes5,
            num_vertices5,
            num_faces5,
            options,
            declaration_normal,
            vertex_size_normal,
            flags5,
            NULL,
            adjacency5,
            (BYTE*)exp_vertices5,
            exp_indices5,
            exp_face_remap5,
            exp_vertex_remap5,
            exp_new_num_vertices5
        },
        {
            (BYTE*)vertices6,
            indices6,
            attributes6,
            num_vertices6,
            num_faces6,
            options,
            declaration_normal,
            vertex_size_normal,
            flags6,
            &epsilons6,
            adjacency6,
            (BYTE*)exp_vertices6,
            exp_indices6,
            exp_face_remap6,
            exp_vertex_remap6,
            exp_new_num_vertices6
        },
        {
            (BYTE*)vertices6,
            (DWORD*)indices6_16bit,
            attributes6,
            num_vertices6,
            num_faces6,
            options_16bit,
            declaration_normal,
            vertex_size_normal,
            flags6,
            &epsilons6,
            adjacency6,
            (BYTE*)exp_vertices6,
            exp_indices6,
            exp_face_remap6,
            exp_vertex_remap6,
            exp_new_num_vertices6
        },
        {
            (BYTE*)vertices8,
            indices8,
            attributes8,
            num_vertices8,
            num_faces8,
            options,
            declaration_normal,
            vertex_size_normal,
            flags8,
            &epsilons8,
            adjacency8,
            (BYTE*)exp_vertices8,
            exp_indices8,
            exp_face_remap8,
            exp_vertex_remap8,
            exp_new_num_vertices8
        },
        {
            (BYTE*)vertices9,
            indices9,
            attributes9,
            num_vertices9,
            num_faces9,
            options,
            declaration_normal,
            vertex_size_normal,
            flags9,
            &epsilons9,
            adjacency9,
            (BYTE*)exp_vertices9,
            exp_indices9,
            exp_face_remap9,
            exp_vertex_remap9,
            exp_new_num_vertices9
        },
        {
            (BYTE*)vertices10,
            indices10,
            attributes10,
            num_vertices10,
            num_faces10,
            options,
            declaration_blendweight,
            vertex_size_blendweight,
            flags10,
            &epsilons10,
            adjacency10,
            (BYTE*)exp_vertices10,
            exp_indices10,
            exp_face_remap10,
            exp_vertex_remap10,
            exp_new_num_vertices10
        },
        {
            (BYTE*)vertices11,
            indices11,
            attributes11,
            num_vertices11,
            num_faces11,
            options,
            declaration_texcoord,
            vertex_size_texcoord,
            flags11,
            &epsilons11,
            adjacency11,
            (BYTE*)exp_vertices11,
            exp_indices11,
            exp_face_remap11,
            exp_vertex_remap11,
            exp_new_num_vertices11
        },
        {
            (BYTE*)vertices12,
            indices12,
            attributes12,
            num_vertices12,
            num_faces12,
            options,
            declaration_color,
            vertex_size_color,
            flags12,
            &epsilons12,
            adjacency12,
            (BYTE*)exp_vertices12,
            exp_indices12,
            exp_face_remap12,
            exp_vertex_remap12,
            exp_new_num_vertices12
        },
        {
            (BYTE*)vertices13,
            indices13,
            attributes13,
            num_vertices13,
            num_faces13,
            options,
            declaration_normal3,
            vertex_size_normal,
            flags13,
            &epsilons13,
            adjacency13,
            (BYTE*)exp_vertices13,
            exp_indices13,
            exp_face_remap13,
            exp_vertex_remap13,
            exp_new_num_vertices13
        },
        {
            (BYTE*)vertices14,
            indices14,
            attributes14,
            num_vertices14,
            num_faces14,
            options,
            declaration_color,
            vertex_size_color,
            flags14,
            &epsilons14,
            adjacency14,
            (BYTE*)exp_vertices14,
            exp_indices14,
            exp_face_remap14,
            exp_vertex_remap14,
            exp_new_num_vertices14
        },
        {
            (BYTE*)vertices15,
            indices15,
            attributes15,
            num_vertices15,
            num_faces15,
            options,
            declaration_color_ubyte4n,
            vertex_size_color_ubyte4, /* UBYTE4 same size as UBYTE4N */
            flags15,
            &epsilons15,
            adjacency15,
            (BYTE*)exp_vertices15,
            exp_indices15,
            exp_face_remap15,
            exp_vertex_remap15,
            exp_new_num_vertices15
        },
        {
            (BYTE*)vertices16,
            indices16,
            attributes16,
            num_vertices16,
            num_faces16,
            options,
            declaration_color_ubyte4,
            vertex_size_color_ubyte4,
            flags16,
            &epsilons16,
            adjacency16,
            (BYTE*)exp_vertices16,
            exp_indices16,
            exp_face_remap16,
            exp_vertex_remap16,
            exp_new_num_vertices16
        },
        {
            (BYTE*)vertices17,
            indices17,
            attributes17,
            num_vertices17,
            num_faces17,
            options,
            declaration_texcoord_short2,
            vertex_size_texcoord_short2,
            flags17,
            &epsilons17,
            adjacency17,
            (BYTE*)exp_vertices17,
            exp_indices17,
            exp_face_remap17,
            exp_vertex_remap17,
            exp_new_num_vertices17
        },
        {
            (BYTE*)vertices18,
            indices18,
            attributes18,
            num_vertices18,
            num_faces18,
            options,
            declaration_texcoord_short2n,
            vertex_size_texcoord_short2, /* SHORT2 same size as SHORT2N */
            flags18,
            &epsilons18,
            adjacency18,
            (BYTE*)exp_vertices18,
            exp_indices18,
            exp_face_remap18,
            exp_vertex_remap18,
            exp_new_num_vertices18
        },
        {
            (BYTE*)vertices19,
            indices19,
            attributes19,
            num_vertices19,
            num_faces19,
            options,
            declaration_texcoord_ushort2n,
            vertex_size_texcoord_short2, /* SHORT2 same size as USHORT2N */
            flags19,
            &epsilons19,
            adjacency19,
            (BYTE*)exp_vertices19,
            exp_indices19,
            exp_face_remap19,
            exp_vertex_remap19,
            exp_new_num_vertices19
        },
        {
            (BYTE*)vertices20,
            indices20,
            attributes20,
            num_vertices20,
            num_faces20,
            options,
            declaration_normal_short4,
            vertex_size_normal_short4,
            flags20,
            &epsilons20,
            adjacency20,
            (BYTE*)exp_vertices20,
            exp_indices20,
            exp_face_remap20,
            exp_vertex_remap20,
            exp_new_num_vertices20
        },
        {
            (BYTE*)vertices21,
            indices21,
            attributes21,
            num_vertices21,
            num_faces21,
            options,
            declaration_normal_short4n,
            vertex_size_normal_short4, /* SHORT4 same size as SHORT4N */
            flags21,
            &epsilons21,
            adjacency21,
            (BYTE*)exp_vertices21,
            exp_indices21,
            exp_face_remap21,
            exp_vertex_remap21,
            exp_new_num_vertices21
        },
        {
            (BYTE*)vertices22,
            indices22,
            attributes22,
            num_vertices22,
            num_faces22,
            options,
            declaration_normal_ushort4n,
            vertex_size_normal_short4, /* SHORT4 same size as USHORT4N */
            flags22,
            &epsilons22,
            adjacency22,
            (BYTE*)exp_vertices22,
            exp_indices22,
            exp_face_remap22,
            exp_vertex_remap22,
            exp_new_num_vertices22
        },
        {
            (BYTE*)vertices23,
            indices23,
            attributes23,
            num_vertices23,
            num_faces23,
            options,
            declaration_texcoord_float16_2,
            vertex_size_texcoord_float16_2,
            flags23,
            &epsilons23,
            adjacency23,
            (BYTE*)exp_vertices23,
            exp_indices23,
            exp_face_remap23,
            exp_vertex_remap23,
            exp_new_num_vertices23
        },
        {
            (BYTE*)vertices24,
            indices24,
            attributes24,
            num_vertices24,
            num_faces24,
            options,
            declaration_texcoord_float16_4,
            vertex_size_texcoord_float16_4,
            flags24,
            &epsilons24,
            adjacency24,
            (BYTE*)exp_vertices24,
            exp_indices24,
            exp_face_remap24,
            exp_vertex_remap24,
            exp_new_num_vertices24
        },
        {
            (BYTE*)vertices25,
            indices25,
            attributes25,
            num_vertices25,
            num_faces25,
            options,
            declaration_texcoord10,
            vertex_size_texcoord,
            flags25,
            &epsilons25,
            adjacency25,
            (BYTE*)exp_vertices25,
            exp_indices25,
            exp_face_remap25,
            exp_vertex_remap25,
            exp_new_num_vertices25
        },
        {
            (BYTE*)vertices26,
            indices26,
            attributes26,
            num_vertices26,
            num_faces26,
            options,
            declaration_color2,
            vertex_size_color,
            flags26,
            &epsilons26,
            adjacency26,
            (BYTE*)exp_vertices26,
            exp_indices26,
            exp_face_remap26,
            exp_vertex_remap26,
            exp_new_num_vertices26
        },
        {
            (BYTE*)vertices27,
            indices27,
            attributes27,
            num_vertices27,
            num_faces27,
            options,
            declaration_color1,
            vertex_size_color,
            flags27,
            &epsilons27,
            adjacency27,
            (BYTE*)exp_vertices27,
            exp_indices27,
            exp_face_remap27,
            exp_vertex_remap27,
            exp_new_num_vertices27
        },
        {
            (BYTE*)vertices28,
            indices28,
            attributes28,
            num_vertices28,
            num_faces28,
            options,
            declaration_normal_udec3,
            vertex_size_normal_udec3,
            flags28,
            &epsilons28,
            adjacency28,
            (BYTE*)exp_vertices28,
            exp_indices28,
            exp_face_remap28,
            exp_vertex_remap28,
            exp_new_num_vertices28
        },
        {
            (BYTE*)vertices29,
            indices29,
            attributes29,
            num_vertices29,
            num_faces29,
            options,
            declaration_normal_dec3n,
            vertex_size_normal_dec3n,
            flags29,
            &epsilons29,
            adjacency29,
            (BYTE*)exp_vertices29,
            exp_indices29,
            exp_face_remap29,
            exp_vertex_remap29,
            exp_new_num_vertices29
        }
    };

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        goto cleanup;
    }

    for (i = 0; i < ARRAY_SIZE(tc); i++)
    {
        DWORD j;
        DWORD *vertex_remap_ptr;
        DWORD new_num_vertices;

        hr = init_test_mesh(tc[i].num_faces, tc[i].num_vertices, tc[i].options,
                            tc[i].declaration, test_context->device, &mesh,
                            tc[i].vertices, tc[i].vertex_size,
                            tc[i].indices, tc[i].attributes);
        if (FAILED(hr))
        {
            skip("Couldn't initialize test mesh %ld.\n", i);
            goto cleanup;
        }

        /* Allocate out parameters */
        adjacency_out = malloc(VERTS_PER_FACE * tc[i].num_faces * sizeof(*adjacency_out));
        if (!adjacency_out)
        {
            skip("Couldn't allocate adjacency_out array.\n");
            goto cleanup;
        }
        face_remap = malloc(tc[i].num_faces * sizeof(*face_remap));
        if (!face_remap)
        {
            skip("Couldn't allocate face_remap array.\n");
            goto cleanup;
        }

        hr = D3DXWeldVertices(mesh, tc[i].flags, tc[i].epsilons, tc[i].adjacency,
                              adjacency_out, face_remap, &vertex_remap);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#lx\n", hr);
        /* Check number of vertices*/
        new_num_vertices = mesh->lpVtbl->GetNumVertices(mesh);
        ok(new_num_vertices == tc[i].exp_new_num_vertices,
           "Mesh %ld: new_num_vertices == %ld, expected %ld.\n",
           i, new_num_vertices, tc[i].exp_new_num_vertices);
        /* Check index buffer */
        if (tc[i].options & D3DXMESH_32BIT)
        {
            hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, (void**)&indices);
            if (FAILED(hr))
            {
                skip("Couldn't lock index buffer.\n");
                goto cleanup;
            }
            for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++)
            {
                ok(indices[j] == tc[i].exp_indices[j],
                   "Mesh %ld: indices[%ld] == %ld, expected %ld\n",
                   i, j, indices[j], tc[i].exp_indices[j]);
            }
        }
        else
        {
            hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, (void**)&indices_16bit);
            if (FAILED(hr))
            {
                skip("Couldn't lock index buffer.\n");
                goto cleanup;
            }
            for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++)
            {
                ok(indices_16bit[j] == tc[i].exp_indices[j],
                   "Mesh %ld: indices_16bit[%ld] == %d, expected %ld\n",
                   i, j, indices_16bit[j], tc[i].exp_indices[j]);
            }
        }
        mesh->lpVtbl->UnlockIndexBuffer(mesh);
        indices = NULL;
        indices_16bit = NULL;
        /* Check adjacency_out */
        for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++)
        {
            ok(adjacency_out[j] == tc[i].adjacency[j],
               "Mesh %ld: adjacency_out[%ld] == %ld, expected %ld\n",
               i, j, adjacency_out[j], tc[i].adjacency[j]);
        }
        /* Check face_remap */
        for (j = 0; j < tc[i].num_faces; j++)
        {
            ok(face_remap[j] == tc[i].exp_face_remap[j],
               "Mesh %ld: face_remap[%ld] == %ld, expected %ld\n",
               i, j, face_remap[j], tc[i].exp_face_remap[j]);
        }
        /* Check vertex_remap */
        vertex_remap_ptr = vertex_remap->lpVtbl->GetBufferPointer(vertex_remap);
        for (j = 0; j < VERTS_PER_FACE * tc[i].num_faces; j++)
        {
            ok(vertex_remap_ptr[j] == tc[i].exp_vertex_remap[j],
               "Mesh %ld: vertex_remap_ptr[%ld] == %ld, expected %ld\n",
               i, j, vertex_remap_ptr[j], tc[i].exp_vertex_remap[j]);
        }
        /* Check vertex buffer */
        hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, (void*)&vertices);
        if (FAILED(hr))
        {
            skip("Couldn't lock vertex buffer.\n");
            goto cleanup;
        }
        /* Check contents of re-ordered vertex buffer */
        for (j = 0; j < tc[i].exp_new_num_vertices; j++)
        {
            int index = tc[i].vertex_size*j;
            check_vertex_components(__LINE__, i, j, &vertices[index], &tc[i].exp_vertices[index], tc[i].declaration);
        }
        mesh->lpVtbl->UnlockVertexBuffer(mesh);
        vertices = NULL;

        /* Free mesh and output data */
        free(adjacency_out);
        adjacency_out = NULL;
        free(face_remap);
        face_remap = NULL;
        vertex_remap->lpVtbl->Release(vertex_remap);
        vertex_remap = NULL;
        mesh->lpVtbl->Release(mesh);
        mesh = NULL;
    }

cleanup:
    free(adjacency_out);
    free(face_remap);
    if (indices) mesh->lpVtbl->UnlockIndexBuffer(mesh);
    if (indices_16bit) mesh->lpVtbl->UnlockIndexBuffer(mesh);
    if (mesh) mesh->lpVtbl->Release(mesh);
    if (vertex_remap) vertex_remap->lpVtbl->Release(vertex_remap);
    if (vertices) mesh->lpVtbl->UnlockVertexBuffer(mesh);
    free_test_context(test_context);
}

static void test_clone_mesh(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    const DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    D3DVERTEXELEMENT9 declaration_pn[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_pntc[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptcn[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_float16_2[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_float16_4[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_float1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_float3[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_float4[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_d3dcolor[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_ubyte4[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_ubyte4n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_short2[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_short4[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_short2n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_short4n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_ushort2n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_USHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_ushort4n[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_USHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_ptc_float16_2_partialu[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_PARTIALU, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_pntc1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    };
    const unsigned int VERTS_PER_FACE = 3;
    BYTE *vertices = NULL;
    INT i;
    struct vertex_pn
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
    };
    struct vertex_pntc
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        D3DXVECTOR2 texcoords;
    };
    struct vertex_ptcn
    {
        D3DXVECTOR3 position;
        D3DXVECTOR2 texcoords;
        D3DXVECTOR3 normal;
    };
    struct vertex_ptc
    {
        D3DXVECTOR3 position;
        D3DXVECTOR2 texcoords;
    };
    struct vertex_ptc_float16_2
    {
        D3DXVECTOR3 position;
        WORD texcoords[2]; /* float16_2 */
    };
    struct vertex_ptc_float16_4
    {
        D3DXVECTOR3 position;
        WORD texcoords[4]; /* float16_4 */
    };
    struct vertex_ptc_float1
    {
        D3DXVECTOR3 position;
        FLOAT texcoords;
    };
    struct vertex_ptc_float3
    {
        D3DXVECTOR3 position;
        FLOAT texcoords[3];
    };
    struct vertex_ptc_float4
    {
        D3DXVECTOR3 position;
        FLOAT texcoords[4];
    };
    struct vertex_ptc_d3dcolor
    {
        D3DXVECTOR3 position;
        BYTE texcoords[4];
    };
    struct vertex_ptc_ubyte4
    {
        D3DXVECTOR3 position;
        BYTE texcoords[4];
    };
    struct vertex_ptc_ubyte4n
    {
        D3DXVECTOR3 position;
        BYTE texcoords[4];
    };
    struct vertex_ptc_short2
    {
        D3DXVECTOR3 position;
        SHORT texcoords[2];
    };
    struct vertex_ptc_short4
    {
        D3DXVECTOR3 position;
        SHORT texcoords[4];
    };
    struct vertex_ptc_ushort2n
    {
        D3DXVECTOR3 position;
        USHORT texcoords[2];
    };
    struct vertex_ptc_ushort4n
    {
        D3DXVECTOR3 position;
        USHORT texcoords[4];
    };
    struct vertex_ptc_udec3
    {
        D3DXVECTOR3 position;
        DWORD texcoords;
    };
    struct vertex_ptc_dec3n
    {
        D3DXVECTOR3 position;
        DWORD texcoords;
    };
    D3DXVECTOR3 up = {0.0f, 0.0f, 1.0f};
    D3DXVECTOR2 zero_vec2 = {0.0f, 0.0f};
    /* Test 0. Check that a mesh can be cloned if the new declaration is the
     * same as the one used to create the mesh.
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_pn vertices0[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},

        {{ 3.0f,  3.0f,  0.f}, up},
        {{ 3.0f,  0.0f,  0.f}, up},
        {{ 1.0f,  0.0f,  0.f}, up},
    };
    const UINT num_vertices0 = ARRAY_SIZE(vertices0);
    const UINT num_faces0 = ARRAY_SIZE(vertices0) / VERTS_PER_FACE;
    const UINT vertex_size0 = sizeof(*vertices0);
    /* Test 1. Check that 16-bit indices are handled. */
    const DWORD options_16bit = D3DXMESH_SYSTEMMEM;
    /* Test 2. Check that the size of each vertex is increased and the data
     * moved if the new declaration adds an element after the original elements.
     */
    const struct vertex_pntc exp_vertices2[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 2.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 0.0f,  0.0f,  0.f}, up, zero_vec2},

        {{ 3.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 3.0f,  0.0f,  0.f}, up, zero_vec2},
        {{ 1.0f,  0.0f,  0.f}, up, zero_vec2},
    };
    const UINT exp_vertex_size2 = sizeof(*exp_vertices2);
    /* Test 3. Check that the size of each vertex is increased and the data
     * moved if the new declaration adds an element between the original
     * elements.
     */
    const struct vertex_ptcn exp_vertices3[] =
    {
        {{ 0.0f,  3.0f,  0.f}, zero_vec2, up},
        {{ 2.0f,  3.0f,  0.f}, zero_vec2, up},
        {{ 0.0f,  0.0f,  0.f}, zero_vec2, up},

        {{ 3.0f,  3.0f,  0.f}, zero_vec2, up},
        {{ 3.0f,  0.0f,  0.f}, zero_vec2, up},
        {{ 1.0f,  0.0f,  0.f}, zero_vec2, up},
    };
    const UINT exp_vertex_size3 = sizeof(*exp_vertices3);
    /* Test 4. Test that data types can be converted, e.g. FLOAT2 to FLOAT16_2. */
    const struct vertex_ptc vertices4[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f}},
    };
    const UINT num_vertices4 = ARRAY_SIZE(vertices4);
    const UINT num_faces4 = ARRAY_SIZE(vertices4) / VERTS_PER_FACE;
    const UINT vertex_size4 = sizeof(*vertices4);
    const struct vertex_ptc_float16_2 exp_vertices4[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a}}, /* {0.5f, 0.7f} */
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd}}, /* {-0.2f, -0.3f} */

        {{ 3.0f,  3.0f,  0.f}, {0x3266, 0x34cd}}, /* {0.2f, 0.3f} */
        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 1.0f,  0.0f,  0.f}, {0x2e66, 0x3266}}, /* {0.1f, 0.2f} */
    };
    const UINT exp_vertex_size4 = sizeof(*exp_vertices4);
    /* Test 5. Convert FLOAT2 to FLOAT16_4. */
    const struct vertex_ptc vertices5[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f}},
    };
    const UINT num_vertices5 = ARRAY_SIZE(vertices5);
    const UINT num_faces5 = ARRAY_SIZE(vertices5) / VERTS_PER_FACE;
    const UINT vertex_size5 = sizeof(*vertices5);
    const struct vertex_ptc_float16_4 exp_vertices5[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00, 0, 0x3c00}}, /* {1.0f, 1.0f, 0.0f, 1.0f} */
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a, 0, 0x3c00}}, /* {0.5f, 0.7f, 0.0f, 1.0f} */
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd, 0, 0x3c00}}, /* {-0.2f, -0.3f, 0.0f, 1.0f} */

        {{ 3.0f,  3.0f,  0.f}, {0x3266, 0x34cd, 0, 0x3c00}}, /* {0.2f, 0.3f, 0.0f, 1.0f} */
        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00, 0, 0x3c00}}, /* {1.0f, 1.0f, 0.0f, 1.0f} */
        {{ 1.0f,  0.0f,  0.f}, {0x2e66, 0x3266, 0, 0x3c00}}, /* {0.1f, 0.2f, 0.0f, 1.0f} */
    };
    const UINT exp_vertex_size5 = sizeof(*exp_vertices5);
    /* Test 6. Convert FLOAT2 to FLOAT1. */
    const struct vertex_ptc vertices6[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f}},
    };
    const UINT num_vertices6 = ARRAY_SIZE(vertices6);
    const UINT num_faces6 = ARRAY_SIZE(vertices6) / VERTS_PER_FACE;
    const UINT vertex_size6 = sizeof(*vertices6);
    const struct vertex_ptc_float1 exp_vertices6[] =
    {
        {{ 0.0f,  3.0f,  0.f},  1.0f},
        {{ 2.0f,  3.0f,  0.f},  0.5f},
        {{ 0.0f,  0.0f,  0.f}, -0.2f},

        {{ 3.0f,  3.0f,  0.f},  0.2f},
        {{ 3.0f,  0.0f,  0.f},  1.0f},
        {{ 1.0f,  0.0f,  0.f},  0.1f},
    };
    const UINT exp_vertex_size6 = sizeof(*exp_vertices6);
    /* Test 7. Convert FLOAT2 to FLOAT3. */
    const struct vertex_ptc vertices7[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f}},
    };
    const UINT num_vertices7 = ARRAY_SIZE(vertices7);
    const UINT num_faces7 = ARRAY_SIZE(vertices7) / VERTS_PER_FACE;
    const UINT vertex_size7 = sizeof(*vertices7);
    const struct vertex_ptc_float3 exp_vertices7[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f, 0.0f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f, 0.0f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f, 0.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f, 0.0f}},
    };
    const UINT exp_vertex_size7 = sizeof(*exp_vertices7);
    /* Test 8. Convert FLOAT2 to FLOAT4. */
    const struct vertex_ptc vertices8[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f}},
    };
    const UINT num_vertices8 = ARRAY_SIZE(vertices8);
    const UINT num_faces8 = ARRAY_SIZE(vertices8) / VERTS_PER_FACE;
    const UINT vertex_size8 = sizeof(*vertices8);
    const struct vertex_ptc_float4 exp_vertices8[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f, 0.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f, 0.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f, 0.0f, 1.0f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f, 0.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f, 0.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f, 0.0f, 1.0f}},
    };
    const UINT exp_vertex_size8 = sizeof(*exp_vertices8);
    /* Test 9. Convert FLOAT2 to D3DCOLOR. */
    const struct vertex_ptc vertices9[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.4f, -0.6f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, { 2.0f, 256.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.11f,  0.2f}},
    };
    const UINT num_vertices9 = ARRAY_SIZE(vertices9);
    const UINT num_faces9 = ARRAY_SIZE(vertices9) / VERTS_PER_FACE;
    const UINT vertex_size9 = sizeof(*vertices9);
    const struct vertex_ptc_d3dcolor exp_vertices9[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 255, 255, 255}},
        {{ 2.0f,  3.0f,  0.f}, {0, 179, 128, 255}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 255}},

        {{ 3.0f,  3.0f,  0.f}, {0, 77, 51, 255}},
        {{ 3.0f,  0.0f,  0.f}, {0, 255, 255, 255}},
        {{ 1.0f,  0.0f,  0.f}, {0, 51, 28, 255}},
    };
    const UINT exp_vertex_size9 = sizeof(*exp_vertices9);
    /* Test 10. Convert FLOAT2 to UBYTE4. */
    const struct vertex_ptc vertices10[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 0.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 2.0f,  3.0f}},
        {{ 0.0f,  0.0f,  0.f}, { 254.0f,  255.0f}},

        {{ 3.0f,  3.0f,  0.f}, { 256.0f, 257.0f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.4f, 1.5f}},
        {{ 1.0f,  0.0f,  0.f}, {-4.0f, -5.0f}},
    };
    const UINT num_vertices10 = ARRAY_SIZE(vertices10);
    const UINT num_faces10 = ARRAY_SIZE(vertices10) / VERTS_PER_FACE;
    const UINT vertex_size10 = sizeof(*vertices10);
    const struct vertex_ptc_ubyte4 exp_vertices10[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 1, 0, 1}},
        {{ 2.0f,  3.0f,  0.f}, {2, 3, 0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {254, 255, 0, 1}},

        {{ 3.0f,  3.0f,  0.f}, {0, 1, 0, 1}},
        {{ 3.0f,  0.0f,  0.f}, {1, 2, 0, 1}},
        {{ 1.0f,  0.0f,  0.f}, {0, 0, 0, 1}},
    };
    const UINT exp_vertex_size10 = sizeof(*exp_vertices10);
    /* Test 11. Convert FLOAT2 to SHORT2. */
    const struct vertex_ptc vertices11[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f, -1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.4f,  0.5f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.5f, -5.0f}},

        {{ 3.0f,  3.0f,  0.f}, {SHRT_MAX, SHRT_MIN}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MAX + 1.0f, SHRT_MIN - 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MAX + 2.0f, SHRT_MIN - 2.0f}},

        {{ 4.0f,  3.0f,  0.f}, {2 * SHRT_MAX, 2 * SHRT_MIN}},
        {{ 6.0f,  0.0f,  0.f}, {3 * SHRT_MAX, 3 * SHRT_MIN}},
        {{ 4.0f,  0.0f,  0.f}, {4 * SHRT_MAX, 4 * SHRT_MIN}},
    };
    const UINT num_vertices11 = ARRAY_SIZE(vertices11);
    const UINT num_faces11 = ARRAY_SIZE(vertices11) / VERTS_PER_FACE;
    const UINT vertex_size11 = sizeof(*vertices11);
    const struct vertex_ptc_short2 exp_vertices11[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {0, -4}},

        {{ 3.0f,  3.0f,  0.f}, {SHRT_MAX, SHRT_MIN + 1}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN, SHRT_MIN}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MIN + 1, SHRT_MAX}},

        {{ 4.0f,  3.0f,  0.f}, {-2, 1}},
        {{ 6.0f,  0.0f,  0.f}, {32765, -32767}},
        {{ 4.0f,  0.0f,  0.f}, {-4, 1}},
    };
    const UINT exp_vertex_size11 = sizeof(*exp_vertices11);
    /* Test 12. Convert FLOAT2 to SHORT4. */
    const struct vertex_ptc vertices12[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f, -1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.4f,  0.5f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.5f, -5.0f}},

        {{ 3.0f,  3.0f,  0.f}, {SHRT_MAX, SHRT_MIN}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MAX + 1.0f, SHRT_MIN - 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MAX + 2.0f, SHRT_MIN - 2.0f}},

        {{ 4.0f,  3.0f,  0.f}, {2 * SHRT_MAX, 2 * SHRT_MIN}},
        {{ 6.0f,  0.0f,  0.f}, {3 * SHRT_MAX, 3 * SHRT_MIN}},
        {{ 4.0f,  0.0f,  0.f}, {4 * SHRT_MAX, 4 * SHRT_MIN}},
    };
    const UINT num_vertices12 = ARRAY_SIZE(vertices12);
    const UINT num_faces12 = ARRAY_SIZE(vertices12) / VERTS_PER_FACE;
    const UINT vertex_size12 = sizeof(*vertices12);
    const struct vertex_ptc_short4 exp_vertices12[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1, 0, 0, 1}},
        {{ 2.0f,  3.0f,  0.f}, {0, 1, 0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {0, -4, 0, 1}},

        {{ 3.0f,  3.0f,  0.f}, {SHRT_MAX, SHRT_MIN + 1, 0, 1}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN, SHRT_MIN, 0, 1}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MIN + 1, SHRT_MAX, 0, 1}},

        {{ 4.0f,  3.0f,  0.f}, {-2, 1, 0, 1}},
        {{ 6.0f,  0.0f,  0.f}, {32765, -32767, 0, 1}},
        {{ 4.0f,  0.0f,  0.f}, {-4, 1, 0, 1}},
    };
    const UINT exp_vertex_size12 = sizeof(*exp_vertices12);
    /* Test 13. Convert FLOAT2 to UBYTE4N. */
    const struct vertex_ptc vertices13[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  2.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.4f, -0.5f}},

        {{ 3.0f,  3.0f,  0.f}, {-0.6f,  -1.0f}},
        {{ 3.0f,  0.0f,  0.f}, {UCHAR_MAX,  UCHAR_MAX + 1}},
        {{ 1.0f,  0.0f,  0.f}, {2 * UCHAR_MAX, -UCHAR_MAX}},
    };
    const UINT num_vertices13 = ARRAY_SIZE(vertices13);
    const UINT num_faces13 = ARRAY_SIZE(vertices13) / VERTS_PER_FACE;
    const UINT vertex_size13 = sizeof(*vertices13);
    const struct vertex_ptc_ubyte4n exp_vertices13[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {255, 255, 0, 255}},
        {{ 2.0f,  3.0f,  0.f}, {128, 179, 0, 255}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 255}},

        {{ 3.0f,  3.0f,  0.f}, {0, 0, 0, 255}},
        {{ 3.0f,  0.0f,  0.f}, {255, 255, 0, 255}},
        {{ 1.0f,  0.0f,  0.f}, {255, 0, 0, 255}},
    };
    const UINT exp_vertex_size13 = sizeof(*exp_vertices13);
    /* Test 14. Convert FLOAT2 to SHORT2N. */
    const struct vertex_ptc vertices14[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f,  2.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.4f,  0.5f}},
        {{ 0.0f,  0.0f,  0.f}, {0.6f, -1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {-0.4f, -0.5f}},
        {{ 3.0f,  0.0f,  0.f}, {-0.9f, -0.99997}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MAX, SHRT_MIN}},
    };
    const UINT num_vertices14 = ARRAY_SIZE(vertices14);
    const UINT num_faces14 = ARRAY_SIZE(vertices14) / VERTS_PER_FACE;
    const UINT vertex_size14 = sizeof(*vertices14);
    const struct vertex_ptc_short2 exp_vertices14[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {SHRT_MAX, SHRT_MAX}},
        {{ 2.0f,  3.0f,  0.f}, {13107, 16384}},
        {{ 0.0f,  0.0f,  0.f}, {19660, SHRT_MIN + 2}},

        {{ 3.0f,  3.0f,  0.f}, {-13106, -16383}},
        {{ 3.0f,  0.0f,  0.f}, {-29489, SHRT_MIN + 3}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MAX, SHRT_MIN + 2}},
    };
    const UINT exp_vertex_size14 = sizeof(*exp_vertices14);
    /* Test 15. Convert FLOAT2 to SHORT4N. */
    const struct vertex_ptc vertices15[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f,  2.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.4f,  0.5f}},
        {{ 0.0f,  0.0f,  0.f}, {0.6f, -1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {-0.4f, -0.5f}},
        {{ 3.0f,  0.0f,  0.f}, {-0.9f, -0.99997}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MAX, SHRT_MIN}},
    };
    const UINT num_vertices15 = ARRAY_SIZE(vertices15);
    const UINT num_faces15 = ARRAY_SIZE(vertices15) / VERTS_PER_FACE;
    const UINT vertex_size15 = sizeof(*vertices15);
    const struct vertex_ptc_short4 exp_vertices15[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {SHRT_MAX, SHRT_MAX, 0, SHRT_MAX}},
        {{ 2.0f,  3.0f,  0.f}, {13107, 16384, 0, SHRT_MAX}},
        {{ 0.0f,  0.0f,  0.f}, {19660, SHRT_MIN + 2, 0, SHRT_MAX}},

        {{ 3.0f,  3.0f,  0.f}, {-13106, -16383, 0, SHRT_MAX}},
        {{ 3.0f,  0.0f,  0.f}, {-29489, SHRT_MIN + 3, 0, SHRT_MAX}},
        {{ 1.0f,  0.0f,  0.f}, {SHRT_MAX, SHRT_MIN + 2, 0, SHRT_MAX}},
    };
    const UINT exp_vertex_size15 = sizeof(*exp_vertices15);
    /* Test 16. Convert FLOAT2 to USHORT2N. */
    const struct vertex_ptc vertices16[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f,  2.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.4f,  0.5f}},
        {{ 0.0f,  0.0f,  0.f}, {0.6f, -1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {-0.4f, -0.5f}},
        {{ 3.0f,  0.0f,  0.f}, {-0.9f,  0.99998f}},
        {{ 1.0f,  0.0f,  0.f}, {USHRT_MAX, 0.0f}},
    };
    const UINT num_vertices16 = ARRAY_SIZE(vertices16);
    const UINT num_faces16 = ARRAY_SIZE(vertices16) / VERTS_PER_FACE;
    const UINT vertex_size16 = sizeof(*vertices16);
    const struct vertex_ptc_ushort2n exp_vertices16[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {USHRT_MAX, USHRT_MAX}},
        {{ 2.0f,  3.0f,  0.f}, {26214, 32768}},
        {{ 0.0f,  0.0f,  0.f}, {39321, 0}},

        {{ 3.0f,  3.0f,  0.f}, {0, 0}},
        {{ 3.0f,  0.0f,  0.f}, {0, USHRT_MAX - 1}},
        {{ 1.0f,  0.0f,  0.f}, {USHRT_MAX, 0}},
    };
    const UINT exp_vertex_size16 = sizeof(*exp_vertices16);
    /* Test 17. Convert FLOAT2 to USHORT4N. */
    const struct vertex_ptc vertices17[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f,  2.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.4f,  0.5f}},
        {{ 0.0f,  0.0f,  0.f}, {0.6f, -1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {-0.4f, -0.5f}},
        {{ 3.0f,  0.0f,  0.f}, {-0.9f,  0.99998f}},
        {{ 1.0f,  0.0f,  0.f}, {USHRT_MAX, 0.0f}},
    };
    const UINT num_vertices17 = ARRAY_SIZE(vertices17);
    const UINT num_faces17 = ARRAY_SIZE(vertices17) / VERTS_PER_FACE;
    const UINT vertex_size17 = sizeof(*vertices17);
    const struct vertex_ptc_ushort4n exp_vertices17[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {USHRT_MAX, USHRT_MAX, 0, USHRT_MAX}},
        {{ 2.0f,  3.0f,  0.f}, {26214, 32768, 0, USHRT_MAX}},
        {{ 0.0f,  0.0f,  0.f}, {39321, 0, 0, USHRT_MAX}},

        {{ 3.0f,  3.0f,  0.f}, {0, 0, 0, USHRT_MAX}},
        {{ 3.0f,  0.0f,  0.f}, {0, USHRT_MAX - 1, 0, USHRT_MAX}},
        {{ 1.0f,  0.0f,  0.f}, {USHRT_MAX, 0, 0, USHRT_MAX}},
    };
    const UINT exp_vertex_size17 = sizeof(*exp_vertices17);
    /* Test 18. Test that the method field is compared by converting a FLOAT2 to
     * FLOAT16_2. where the method field has been change from
     * D3DDECLMETHOD_DEFAULT to D3DDECLMETHOD_PARTIALU. */
    const struct vertex_ptc vertices18[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f,  0.2f}},
    };
    const UINT num_vertices18 = ARRAY_SIZE(vertices18);
    const UINT num_faces18 = ARRAY_SIZE(vertices18) / VERTS_PER_FACE;
    const UINT vertex_size18 = sizeof(*vertices18);
    const struct vertex_ptc_float16_2 exp_vertices18[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a}}, /* {0.5f, 0.7f} */
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd}}, /* {-0.2f, -0.3f} */

        {{ 3.0f,  3.0f,  0.f}, {0x3266, 0x34cd}}, /* {0.2f, 0.3f} */
        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 1.0f,  0.0f,  0.f}, {0x2e66, 0x3266}}, /* {0.1f, 0.2f} */
    };
    const UINT exp_vertex_size18 = sizeof(*exp_vertices18);
    /* Test 19. Test that data is lost if usage index changes, e.g. TEXCOORD0
     * TEXCOORD1. */
    const struct vertex_pntc vertices19[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, up, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, up, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, up, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, up, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, up, { 0.1f,  0.2f}},
    };
    const UINT num_vertices19 = ARRAY_SIZE(vertices19);
    const UINT num_faces19 = ARRAY_SIZE(vertices19) / VERTS_PER_FACE;
    const UINT vertex_size19 = sizeof(*vertices19);
    const struct vertex_pntc exp_vertices19[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 2.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 0.0f,  0.0f,  0.f}, up, zero_vec2},

        {{ 3.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 3.0f,  0.0f,  0.f}, up, zero_vec2},
        {{ 1.0f,  0.0f,  0.f}, up, zero_vec2},
    };
    const UINT exp_vertex_size19 = sizeof(*exp_vertices19);
    /* Test 20. Another test that data is lost if usage index changes, e.g.
     * TEXCOORD1 to TEXCOORD0. */
    const struct vertex_pntc vertices20[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, { 1.0f,  1.0f}},
        {{ 2.0f,  3.0f,  0.f}, up, { 0.5f,  0.7f}},
        {{ 0.0f,  0.0f,  0.f}, up, {-0.2f, -0.3f}},

        {{ 3.0f,  3.0f,  0.f}, up, { 0.2f,  0.3f}},
        {{ 3.0f,  0.0f,  0.f}, up, { 1.0f,  1.0f}},
        {{ 1.0f,  0.0f,  0.f}, up, { 0.1f,  0.2f}},
    };
    const UINT num_vertices20 = ARRAY_SIZE(vertices20);
    const UINT num_faces20 = ARRAY_SIZE(vertices20) / VERTS_PER_FACE;
    const UINT vertex_size20 = sizeof(*vertices20);
    const struct vertex_pntc exp_vertices20[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 2.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 0.0f,  0.0f,  0.f}, up, zero_vec2},

        {{ 3.0f,  3.0f,  0.f}, up, zero_vec2},
        {{ 3.0f,  0.0f,  0.f}, up, zero_vec2},
        {{ 1.0f,  0.0f,  0.f}, up, zero_vec2},
    };
    const UINT exp_vertex_size20 = sizeof(*exp_vertices20);
    /* Test 21. Convert FLOAT1 to FLOAT2. */
    const struct vertex_ptc_float1 vertices21[] =
    {
        {{ 0.0f,  3.0f,  0.f},  1.0f},
        {{ 2.0f,  3.0f,  0.f},  0.5f},
        {{ 0.0f,  0.0f,  0.f}, -0.2f},

        {{ 3.0f,  3.0f,  0.f},  0.2f},
        {{ 3.0f,  0.0f,  0.f},  1.0f},
        {{ 1.0f,  0.0f,  0.f},  0.1f},
    };
    const UINT num_vertices21 = ARRAY_SIZE(vertices21);
    const UINT num_faces21 = ARRAY_SIZE(vertices21) / VERTS_PER_FACE;
    const UINT vertex_size21 = sizeof(*vertices21);
    const struct vertex_ptc exp_vertices21[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f, 0.0f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f, 0.0f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f, 0.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f, 0.0f}},
    };
    const UINT exp_vertex_size21 = sizeof(*exp_vertices21);
    /* Test 22. Convert FLOAT1 to FLOAT3. */
    const struct vertex_ptc_float1 vertices22[] =
    {
        {{ 0.0f,  3.0f,  0.f},  1.0f},
        {{ 2.0f,  3.0f,  0.f},  0.5f},
        {{ 0.0f,  0.0f,  0.f}, -0.2f},

        {{ 3.0f,  3.0f,  0.f},  0.2f},
        {{ 3.0f,  0.0f,  0.f},  1.0f},
        {{ 1.0f,  0.0f,  0.f},  0.1f},
    };
    const UINT num_vertices22 = ARRAY_SIZE(vertices22);
    const UINT num_faces22 = ARRAY_SIZE(vertices22) / VERTS_PER_FACE;
    const UINT vertex_size22 = sizeof(*vertices22);
    const struct vertex_ptc_float3 exp_vertices22[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f, 0.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f, 0.0f, 0.0f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, 0.0f, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f, 0.0f, 0.0f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f, 0.0f, 0.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f, 0.0f, 0.0f}},
    };
    const UINT exp_vertex_size22 = sizeof(*exp_vertices22);
    /* Test 23. Convert FLOAT1 to FLOAT4. */
    const struct vertex_ptc_float1 vertices23[] =
    {
        {{ 0.0f,  3.0f,  0.f},  1.0f},
        {{ 2.0f,  3.0f,  0.f},  0.5f},
        {{ 0.0f,  0.0f,  0.f}, -0.2f},

        {{ 3.0f,  3.0f,  0.f},  0.2f},
        {{ 3.0f,  0.0f,  0.f},  1.0f},
        {{ 1.0f,  0.0f,  0.f},  0.1f},
    };
    const UINT num_vertices23 = ARRAY_SIZE(vertices23);
    const UINT num_faces23 = ARRAY_SIZE(vertices23) / VERTS_PER_FACE;
    const UINT vertex_size23 = sizeof(*vertices23);
    const struct vertex_ptc_float4 exp_vertices23[] =
    {
        {{ 0.0f,  3.0f,  0.f}, { 1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, { 0.5f, 0.0f, 0.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.2f, 0.0f, 0.0f, 1.0f}},

        {{ 3.0f,  3.0f,  0.f}, { 0.2f, 0.0f, 0.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, { 1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, { 0.1f, 0.0f, 0.0f, 1.0f}},
    };
    const UINT exp_vertex_size23 = sizeof(*exp_vertices23);
    /* Test 24. Convert FLOAT1 to D3DCOLOR. */
    const struct vertex_ptc_float1 vertices24[] =
    {
        {{ 0.0f,  3.0f,  0.f},  1.0f},
        {{ 2.0f,  3.0f,  0.f},  0.5f},
        {{ 0.0f,  0.0f,  0.f}, -0.2f},

        {{ 3.0f,  3.0f,  0.f},  0.2f},
        {{ 3.0f,  0.0f,  0.f},  1.0f},
        {{ 1.0f,  0.0f,  0.f},  0.11f},
    };
    const UINT num_vertices24 = ARRAY_SIZE(vertices24);
    const UINT num_faces24 = ARRAY_SIZE(vertices24) / VERTS_PER_FACE;
    const UINT vertex_size24 = sizeof(*vertices24);
    const struct vertex_ptc_d3dcolor exp_vertices24[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 255, 255}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 128, 255}},
        {{ 0.0f,  0.0f,  0.f}, {0, 0, 0, 255}},

        {{ 3.0f,  3.0f,  0.f}, {0, 0, 51, 255}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 255, 255}},
        {{ 1.0f,  0.0f,  0.f}, {0, 0, 28, 255}},
    };
    const UINT exp_vertex_size24 = sizeof(*exp_vertices24);
    /* Test 25. Convert FLOAT1 to ubyte4. */
    const struct vertex_ptc_float1 vertices25[] =
    {
        {{ 0.0f,  3.0f,  0.f}, 0.0f},
        {{ 2.0f,  3.0f,  0.f}, 1.4f},
        {{ 0.0f,  0.0f,  0.f}, 1.5f},

        {{ 3.0f,  3.0f,  0.f}, 255.0f},
        {{ 3.0f,  0.0f,  0.f}, 256.0f},
        {{ 1.0f,  0.0f,  0.f}, -1.0f},
    };
    const UINT num_vertices25 = ARRAY_SIZE(vertices25);
    const UINT num_faces25 = ARRAY_SIZE(vertices25) / VERTS_PER_FACE;
    const UINT vertex_size25 = sizeof(*vertices25);
    const struct vertex_ptc_ubyte4 exp_vertices25[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 1}},
        {{ 2.0f,  3.0f,  0.f}, {1, 0, 0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {2, 0, 0, 1}},

        {{ 3.0f,  3.0f,  0.f}, {255, 0, 0, 1}},
        {{ 3.0f,  0.0f,  0.f}, {0, 0, 0, 1}},
        {{ 1.0f,  0.0f,  0.f}, {0, 0, 0, 1}},
    };
    const UINT exp_vertex_size25 = sizeof(*exp_vertices25);
    /* Test 26. Convert FLOAT4 to D3DCOLOR. */
    const struct vertex_ptc_float4 vertices26[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 1.0f, 0.4f, 0.5f}},
        {{ 2.0f,  3.0f,  0.f}, {-0.4f, -0.5f, -1.0f, -2.0f}},
        {{ 0.0f,  0.0f,  0.f}, {254.0f, 255.0f, 256.0f, 257.0f}},

        {{ 3.0f,  3.0f,  0.f}, {0.1f, 0.2f, 0.3f, 0.4f}},
        {{ 3.0f,  0.0f,  0.f}, {0.5f, 0.6f, 0.7f, 0.8f}},
        {{ 1.0f,  0.0f,  0.f}, {0.9f, 0.99f, 0.995f, 0.999f}},
    };
    const UINT num_vertices26 = ARRAY_SIZE(vertices26);
    const UINT num_faces26 = ARRAY_SIZE(vertices26) / VERTS_PER_FACE;
    const UINT vertex_size26 = sizeof(*vertices26);
    const struct vertex_ptc_d3dcolor exp_vertices26[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {102, 255, 0, 128}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {255, 255, 255, 255}},

        {{ 3.0f,  3.0f,  0.f}, {77, 51, 26, 102}},
        {{ 3.0f,  0.0f,  0.f}, {179, 153, 128, 204}},
        {{ 1.0f,  0.0f,  0.f}, {254, 252, 230, 255}},
    };
    const UINT exp_vertex_size26 = sizeof(*exp_vertices26);
    /* Test 27. Convert D3DCOLOR to FLOAT4. */
    const struct vertex_ptc_d3dcolor vertices27[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {102, 255, 0, 128}},
        {{ 2.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 0.0f,  0.0f,  0.f}, {255, 255, 255, 255}},

        {{ 3.0f,  3.0f,  0.f}, {77, 51, 26, 102}},
        {{ 3.0f,  0.0f,  0.f}, {179, 153, 128, 204}},
        {{ 1.0f,  0.0f,  0.f}, {254, 252, 230, 255}},
    };
    const UINT num_vertices27 = ARRAY_SIZE(vertices27);
    const UINT num_faces27 = ARRAY_SIZE(vertices27) / VERTS_PER_FACE;
    const UINT vertex_size27 = sizeof(*vertices27);
    const struct vertex_ptc_float4 exp_vertices27[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 1.0f, 0.4f, 0.501961f}},
        {{ 2.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{ 0.0f,  0.0f,  0.f}, {1.0f, 1.0f, 1.0f, 1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {0.101961f, 0.2f, 0.301961f, 0.4f}},
        {{ 3.0f,  0.0f,  0.f}, {0.501961f, 0.6f, 0.701961f, 0.8f}},
        {{ 1.0f,  0.0f,  0.f}, {0.901961f, 0.988235f, 0.996078f, 1.0f}},
    };
    const UINT exp_vertex_size27 = sizeof(*exp_vertices27);
    /* Test 28. Convert UBYTE4 to FLOAT4. */
    const struct vertex_ptc_ubyte4 vertices28[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {1, 1, 1, 1}},
        {{ 0.0f,  0.0f,  0.f}, {1, 0, 1, 0}},

        {{ 3.0f,  3.0f,  0.f}, {0, 1, 0, 1}},
        {{ 3.0f,  0.0f,  0.f}, {10, 20, 30, 40}},
        {{ 1.0f,  0.0f,  0.f}, {50, 60, 127, 255}},
    };
    const UINT num_vertices28 = ARRAY_SIZE(vertices28);
    const UINT num_faces28 = ARRAY_SIZE(vertices28) / VERTS_PER_FACE;
    const UINT vertex_size28 = sizeof(*vertices28);
    const struct vertex_ptc_float4 exp_vertices28[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {1.0f,  0.0f, 1.0f, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, {10.0f, 20.0f, 30.0f, 40.0f}},
        {{ 1.0f,  0.0f,  0.f}, {50.0f, 60.0f, 127.0f, 255.0f}},
    };
    const UINT exp_vertex_size28 = sizeof(*exp_vertices28);
    /* Test 29. Convert SHORT2 to FLOAT4. */
    const struct vertex_ptc_short2 vertices29[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {1, 0}},

        {{ 3.0f,  3.0f,  0.f}, {1, 1}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN, SHRT_MAX}},
        {{ 1.0f,  0.0f,  0.f}, {-42, 42}},
    };
    const UINT num_vertices29 = ARRAY_SIZE(vertices29);
    const UINT num_faces29 = ARRAY_SIZE(vertices29) / VERTS_PER_FACE;
    const UINT vertex_size29 = sizeof(*vertices29);
    const struct vertex_ptc_float4 exp_vertices29[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.0f, 1.0f, 0.0f, 1.0f }},
        {{ 0.0f,  0.0f,  0.f}, {1.0f, 0.0f, 0.0f, 1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN, SHRT_MAX, 0.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {-42.0f, 42.0f, 0.0f, 1.0f}},
    };
    const UINT exp_vertex_size29 = sizeof(*exp_vertices29);
    /* Test 29. Convert SHORT4 to FLOAT4. */
    const struct vertex_ptc_short4 vertices30[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 1, 0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {1, 0, 1, 0}},

        {{ 3.0f,  3.0f,  0.f}, {1, 1, 1, 1}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN, SHRT_MAX, 1, 0}},
        {{ 1.0f,  0.0f,  0.f}, {-42, 42, SHRT_MAX, SHRT_MIN}},
    };
    const UINT num_vertices30 = ARRAY_SIZE(vertices30);
    const UINT num_faces30 = ARRAY_SIZE(vertices30) / VERTS_PER_FACE;
    const UINT vertex_size30 = sizeof(*vertices30);
    const struct vertex_ptc_float4 exp_vertices30[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.0f, 1.0f, 0.0f, 1.0f }},
        {{ 0.0f,  0.0f,  0.f}, {1.0f, 0.0f, 1.0f, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN, SHRT_MAX, 1.0f, 0.0f}},
        {{ 1.0f,  0.0f,  0.f}, {-42.0f, 42.0f, SHRT_MAX, SHRT_MIN}},
    };
    const UINT exp_vertex_size30 = sizeof(*exp_vertices30);
    /* Test 31. Convert UBYTE4N to FLOAT4. */
    const struct vertex_ptc_ubyte4n vertices31[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {1, 1, 1, 1}},
        {{ 0.0f,  0.0f,  0.f}, {1, 0, 1, 0}},

        {{ 3.0f,  3.0f,  0.f}, {0, 1, 0, 1}},
        {{ 3.0f,  0.0f,  0.f}, {10, 20, 30, 40}},
        {{ 1.0f,  0.0f,  0.f}, {50, 60, 70, UCHAR_MAX}},
    };
    const UINT num_vertices31 = ARRAY_SIZE(vertices31);
    const UINT num_faces31 = ARRAY_SIZE(vertices31) / VERTS_PER_FACE;
    const UINT vertex_size31 = sizeof(*vertices31);
    const struct vertex_ptc_float4 exp_vertices31[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, {(FLOAT)1/UCHAR_MAX, (FLOAT)1/UCHAR_MAX, (FLOAT)1/UCHAR_MAX, (FLOAT)1/UCHAR_MAX}},
        {{ 0.0f,  0.0f,  0.f}, {(FLOAT)1/UCHAR_MAX, 0.0f, (FLOAT)1/UCHAR_MAX, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, (FLOAT)1/UCHAR_MAX, 0.0f, (FLOAT)1/UCHAR_MAX}},
        {{ 3.0f,  0.0f,  0.f}, {(FLOAT)10/UCHAR_MAX, (FLOAT)20/UCHAR_MAX, (FLOAT)30/UCHAR_MAX, (FLOAT)40/UCHAR_MAX}},
        {{ 1.0f,  0.0f,  0.f}, {(FLOAT)50/UCHAR_MAX, (FLOAT)60/UCHAR_MAX, (FLOAT)70/UCHAR_MAX, 1.0f}},
    };
    const UINT exp_vertex_size31 = sizeof(*exp_vertices31);
    /* Test 32. Convert SHORT2N to FLOAT4. */
    const struct vertex_ptc_short2 vertices32[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {1, 0}},

        {{ 3.0f,  3.0f,  0.f}, {1, 1}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN + 1, SHRT_MAX}},
        {{ 1.0f,  0.0f,  0.f}, {-42, 42}},
    };
    const UINT num_vertices32 = ARRAY_SIZE(vertices32);
    const UINT num_faces32 = ARRAY_SIZE(vertices32) / VERTS_PER_FACE;
    const UINT vertex_size32 = sizeof(*vertices32);
    const struct vertex_ptc_float4 exp_vertices32[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.0f, 1.0f/SHRT_MAX, 0.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {1.0f/SHRT_MAX, 0.0f, 0.0f, 1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {1.0f/SHRT_MAX, 1.0f/SHRT_MAX, 0.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, {-1.0f, 1.0f, 0.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {-42.0f/SHRT_MAX, 42.0f/SHRT_MAX, 0.0f, 1.0f}},
    };
    const UINT exp_vertex_size32 = sizeof(*exp_vertices32);
    /* Test 33. Convert SHORT4N to FLOAT4. */
    const struct vertex_ptc_short4 vertices33[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0, 0, 0, 0}},
        {{ 2.0f,  3.0f,  0.f}, {0, 1, 0, 1}},
        {{ 0.0f,  0.0f,  0.f}, {1, 0, 1, 0}},

        {{ 3.0f,  3.0f,  0.f}, {1, 1, 1, 1}},
        {{ 3.0f,  0.0f,  0.f}, {SHRT_MIN + 1, SHRT_MAX, SHRT_MIN + 1, SHRT_MAX}},
        {{ 1.0f,  0.0f,  0.f}, {-42, 42, 1, 1}},
    };
    const UINT num_vertices33 = ARRAY_SIZE(vertices33);
    const UINT num_faces33 = ARRAY_SIZE(vertices33) / VERTS_PER_FACE;
    const UINT vertex_size33 = sizeof(*vertices33);
    const struct vertex_ptc_float4 exp_vertices33[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.0f, 1.0f/SHRT_MAX, 0.0f, 1.0f/SHRT_MAX}},
        {{ 0.0f,  0.0f,  0.f}, {1.0f/SHRT_MAX, 0.0f, 1.0f/SHRT_MAX, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, {1.0f/SHRT_MAX, 1.0f/SHRT_MAX, 1.0f/SHRT_MAX, 1.0f/SHRT_MAX}},
        {{ 3.0f,  0.0f,  0.f}, {-1.0f, 1.0f, -1.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {-42.0f/SHRT_MAX, 42.0f/SHRT_MAX, 1.0f/SHRT_MAX, 1.0f/SHRT_MAX}},
    };
    const UINT exp_vertex_size33 = sizeof(*exp_vertices33);
    /* Test 34. Convert FLOAT16_2 to FLOAT4. */
    const struct vertex_ptc_float16_2 vertices34[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 2.0f,  3.0f,  0.f}, {0x3800, 0x399a}}, /* {0.5f, 0.7f} */
        {{ 0.0f,  0.0f,  0.f}, {0xb266, 0xb4cd}}, /* {-0.2f, -0.3f} */

        {{ 3.0f,  3.0f,  0.f}, {0x3266, 0x34cd}}, /* {0.2f, 0.3f} */
        {{ 3.0f,  0.0f,  0.f}, {0x3c00, 0x3c00}}, /* {1.0f, 1.0f} */
        {{ 1.0f,  0.0f,  0.f}, {0x2e66, 0x3266}}, /* {0.1f, 0.2f} */
    };
    const UINT num_vertices34 = ARRAY_SIZE(vertices34);
    const UINT num_faces34 = ARRAY_SIZE(vertices34) / VERTS_PER_FACE;
    const UINT vertex_size34 = sizeof(*vertices34);
    const struct vertex_ptc_float4 exp_vertices34[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{ 2.0f,  3.0f,  0.f}, {0.5f, 0.700195f, 0.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {-0.199951f, -0.300049f, 0.0f, 1.0f}},

        {{ 3.0f,  3.0f,  0.f}, {0.199951f, 0.300049f, 0.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{ 1.0f,  0.0f,  0.f}, {0.099976f, 0.199951f, 0.0f, 1.0f}},
    };
    const UINT exp_vertex_size34 = sizeof(*exp_vertices34);
    /* Test 35. Convert FLOAT16_4 to FLOAT4. */
    const struct vertex_ptc_float16_4 vertices35[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0x0000, 0x0000, 0x0000, 0x0000}},
        {{ 2.0f,  3.0f,  0.f}, {0x3c00, 0x3c00, 0x3c00, 0x3c00}},
        {{ 0.0f,  0.0f,  0.f}, {0x3c00, 0x0000, 0x3c00, 0x0000}},

        {{ 3.0f,  3.0f,  0.f}, {0x0000, 0x3c00, 0x0000, 0x3c00}},
        {{ 3.0f,  0.0f,  0.f}, {0x3800, 0x399a, 0xb266, 0xb4cd}},
        {{ 1.0f,  0.0f,  0.f}, {0x2e66, 0x3266, 0x2e66, 0x3266}},
    };
    const UINT num_vertices35 = ARRAY_SIZE(vertices35);
    const UINT num_faces35 = ARRAY_SIZE(vertices35) / VERTS_PER_FACE;
    const UINT vertex_size35 = sizeof(*vertices35);
    const struct vertex_ptc_float4 exp_vertices35[] =
    {
        {{ 0.0f,  3.0f,  0.f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{ 2.0f,  3.0f,  0.f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        {{ 0.0f,  0.0f,  0.f}, {1.0f, 0.0f, 1.0f, 0.0f}},

        {{ 3.0f,  3.0f,  0.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{ 3.0f,  0.0f,  0.f}, {0.5f, 0.700195f, -0.199951f, -0.300049f}},
        {{ 1.0f,  0.0f,  0.f}, {0.099976f, 0.199951f, 0.099976f, 0.199951f}},
    };
    const UINT exp_vertex_size35 = sizeof(*exp_vertices35);
    /* Test 36. Check that vertex buffer sharing is ok. */
    const struct vertex_pn vertices36[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up},
        {{ 2.0f,  3.0f,  0.f}, up},
        {{ 0.0f,  0.0f,  0.f}, up},
    };
    const UINT num_vertices36 = ARRAY_SIZE(vertices36);
    const UINT num_faces36 = ARRAY_SIZE(vertices36) / VERTS_PER_FACE;
    const UINT vertex_size36 = sizeof(*vertices36);
    const DWORD clone_options36 = options | D3DXMESH_VB_SHARE;
    /* Common mesh data */
    ID3DXMesh *mesh = NULL;
    ID3DXMesh *mesh_clone = NULL;
    struct
    {
        const BYTE *vertices;
        const DWORD *indices;
        const DWORD *attributes;
        const UINT num_vertices;
        const UINT num_faces;
        const UINT vertex_size;
        const DWORD create_options;
        const DWORD clone_options;
        D3DVERTEXELEMENT9 *declaration;
        D3DVERTEXELEMENT9 *new_declaration;
        const BYTE *exp_vertices;
        const UINT exp_vertex_size;
    }
    tc[] =
    {
        {
            (BYTE*)vertices0,
            NULL,
            NULL,
            num_vertices0,
            num_faces0,
            vertex_size0,
            options,
            options,
            declaration_pn,
            declaration_pn,
            (BYTE*)vertices0,
            vertex_size0
        },
        {
            (BYTE*)vertices0,
            NULL,
            NULL,
            num_vertices0,
            num_faces0,
            vertex_size0,
            options_16bit,
            options_16bit,
            declaration_pn,
            declaration_pn,
            (BYTE*)vertices0,
            vertex_size0
        },
        {
            (BYTE*)vertices0,
            NULL,
            NULL,
            num_vertices0,
            num_faces0,
            vertex_size0,
            options,
            options,
            declaration_pn,
            declaration_pntc,
            (BYTE*)exp_vertices2,
            exp_vertex_size2
        },
        {
            (BYTE*)vertices0,
            NULL,
            NULL,
            num_vertices0,
            num_faces0,
            vertex_size0,
            options,
            options,
            declaration_pn,
            declaration_ptcn,
            (BYTE*)exp_vertices3,
            exp_vertex_size3
        },
        {
            (BYTE*)vertices4,
            NULL,
            NULL,
            num_vertices4,
            num_faces4,
            vertex_size4,
            options,
            options,
            declaration_ptc,
            declaration_ptc_float16_2,
            (BYTE*)exp_vertices4,
            exp_vertex_size4
        },
        {
            (BYTE*)vertices5,
            NULL,
            NULL,
            num_vertices5,
            num_faces5,
            vertex_size5,
            options,
            options,
            declaration_ptc,
            declaration_ptc_float16_4,
            (BYTE*)exp_vertices5,
            exp_vertex_size5
        },
        {
            (BYTE*)vertices6,
            NULL,
            NULL,
            num_vertices6,
            num_faces6,
            vertex_size6,
            options,
            options,
            declaration_ptc,
            declaration_ptc_float1,
            (BYTE*)exp_vertices6,
            exp_vertex_size6
        },
        {
            (BYTE*)vertices7,
            NULL,
            NULL,
            num_vertices7,
            num_faces7,
            vertex_size7,
            options,
            options,
            declaration_ptc,
            declaration_ptc_float3,
            (BYTE*)exp_vertices7,
            exp_vertex_size7
        },
        {
            (BYTE*)vertices8,
            NULL,
            NULL,
            num_vertices8,
            num_faces8,
            vertex_size8,
            options,
            options,
            declaration_ptc,
            declaration_ptc_float4,
            (BYTE*)exp_vertices8,
            exp_vertex_size8
        },
        {
            (BYTE*)vertices9,
            NULL,
            NULL,
            num_vertices9,
            num_faces9,
            vertex_size9,
            options,
            options,
            declaration_ptc,
            declaration_ptc_d3dcolor,
            (BYTE*)exp_vertices9,
            exp_vertex_size9
        },
        {
            (BYTE*)vertices10,
            NULL,
            NULL,
            num_vertices10,
            num_faces10,
            vertex_size10,
            options,
            options,
            declaration_ptc,
            declaration_ptc_ubyte4,
            (BYTE*)exp_vertices10,
            exp_vertex_size10
        },
        {
            (BYTE*)vertices11,
            NULL,
            NULL,
            num_vertices11,
            num_faces11,
            vertex_size11,
            options,
            options,
            declaration_ptc,
            declaration_ptc_short2,
            (BYTE*)exp_vertices11,
            exp_vertex_size11
        },
        {
            (BYTE*)vertices12,
            NULL,
            NULL,
            num_vertices12,
            num_faces12,
            vertex_size12,
            options,
            options,
            declaration_ptc,
            declaration_ptc_short4,
            (BYTE*)exp_vertices12,
            exp_vertex_size12
        },
        {
            (BYTE*)vertices13,
            NULL,
            NULL,
            num_vertices13,
            num_faces13,
            vertex_size13,
            options,
            options,
            declaration_ptc,
            declaration_ptc_ubyte4n,
            (BYTE*)exp_vertices13,
            exp_vertex_size13
        },
        {
            (BYTE*)vertices14,
            NULL,
            NULL,
            num_vertices14,
            num_faces14,
            vertex_size14,
            options,
            options,
            declaration_ptc,
            declaration_ptc_short2n,
            (BYTE*)exp_vertices14,
            exp_vertex_size14
        },
        {
            (BYTE*)vertices15,
            NULL,
            NULL,
            num_vertices15,
            num_faces15,
            vertex_size15,
            options,
            options,
            declaration_ptc,
            declaration_ptc_short4n,
            (BYTE*)exp_vertices15,
            exp_vertex_size15
        },
        {
            (BYTE*)vertices16,
            NULL,
            NULL,
            num_vertices16,
            num_faces16,
            vertex_size16,
            options,
            options,
            declaration_ptc,
            declaration_ptc_ushort2n,
            (BYTE*)exp_vertices16,
            exp_vertex_size16
        },
        {
            (BYTE*)vertices17,
            NULL,
            NULL,
            num_vertices17,
            num_faces17,
            vertex_size17,
            options,
            options,
            declaration_ptc,
            declaration_ptc_ushort4n,
            (BYTE*)exp_vertices17,
            exp_vertex_size17
        },
        {
            (BYTE*)vertices18,
            NULL,
            NULL,
            num_vertices18,
            num_faces18,
            vertex_size18,
            options,
            options,
            declaration_ptc,
            declaration_ptc_float16_2_partialu,
            (BYTE*)exp_vertices18,
            exp_vertex_size18
        },
        {
            (BYTE*)vertices19,
            NULL,
            NULL,
            num_vertices19,
            num_faces19,
            vertex_size19,
            options,
            options,
            declaration_pntc,
            declaration_pntc1,
            (BYTE*)exp_vertices19,
            exp_vertex_size19
        },
        {
            (BYTE*)vertices20,
            NULL,
            NULL,
            num_vertices20,
            num_faces20,
            vertex_size20,
            options,
            options,
            declaration_pntc1,
            declaration_pntc,
            (BYTE*)exp_vertices20,
            exp_vertex_size20
        },
        {
            (BYTE*)vertices21,
            NULL,
            NULL,
            num_vertices21,
            num_faces21,
            vertex_size21,
            options,
            options,
            declaration_ptc_float1,
            declaration_ptc,
            (BYTE*)exp_vertices21,
            exp_vertex_size21
        },
        {
            (BYTE*)vertices22,
            NULL,
            NULL,
            num_vertices22,
            num_faces22,
            vertex_size22,
            options,
            options,
            declaration_ptc_float1,
            declaration_ptc_float3,
            (BYTE*)exp_vertices22,
            exp_vertex_size22
        },
        {
            (BYTE*)vertices23,
            NULL,
            NULL,
            num_vertices23,
            num_faces23,
            vertex_size23,
            options,
            options,
            declaration_ptc_float1,
            declaration_ptc_float4,
            (BYTE*)exp_vertices23,
            exp_vertex_size23
        },
        {
            (BYTE*)vertices24,
            NULL,
            NULL,
            num_vertices24,
            num_faces24,
            vertex_size24,
            options,
            options,
            declaration_ptc_float1,
            declaration_ptc_d3dcolor,
            (BYTE*)exp_vertices24,
            exp_vertex_size24
        },
        {
            (BYTE*)vertices25,
            NULL,
            NULL,
            num_vertices25,
            num_faces25,
            vertex_size25,
            options,
            options,
            declaration_ptc_float1,
            declaration_ptc_ubyte4,
            (BYTE*)exp_vertices25,
            exp_vertex_size25
        },
        {
            (BYTE*)vertices26,
            NULL,
            NULL,
            num_vertices26,
            num_faces26,
            vertex_size26,
            options,
            options,
            declaration_ptc_float4,
            declaration_ptc_d3dcolor,
            (BYTE*)exp_vertices26,
            exp_vertex_size26
        },
        {
            (BYTE*)vertices27,
            NULL,
            NULL,
            num_vertices27,
            num_faces27,
            vertex_size27,
            options,
            options,
            declaration_ptc_d3dcolor,
            declaration_ptc_float4,
            (BYTE*)exp_vertices27,
            exp_vertex_size27
        },
        {
            (BYTE*)vertices28,
            NULL,
            NULL,
            num_vertices28,
            num_faces28,
            vertex_size28,
            options,
            options,
            declaration_ptc_ubyte4,
            declaration_ptc_float4,
            (BYTE*)exp_vertices28,
            exp_vertex_size28
        },
        {
            (BYTE*)vertices29,
            NULL,
            NULL,
            num_vertices29,
            num_faces29,
            vertex_size29,
            options,
            options,
            declaration_ptc_short2,
            declaration_ptc_float4,
            (BYTE*)exp_vertices29,
            exp_vertex_size29
        },
        {
            (BYTE*)vertices30,
            NULL,
            NULL,
            num_vertices30,
            num_faces30,
            vertex_size30,
            options,
            options,
            declaration_ptc_short4,
            declaration_ptc_float4,
            (BYTE*)exp_vertices30,
            exp_vertex_size30
        },
        {
            (BYTE*)vertices31,
            NULL,
            NULL,
            num_vertices31,
            num_faces31,
            vertex_size31,
            options,
            options,
            declaration_ptc_ubyte4n,
            declaration_ptc_float4,
            (BYTE*)exp_vertices31,
            exp_vertex_size31
        },
        {
            (BYTE*)vertices32,
            NULL,
            NULL,
            num_vertices32,
            num_faces32,
            vertex_size32,
            options,
            options,
            declaration_ptc_short2n,
            declaration_ptc_float4,
            (BYTE*)exp_vertices32,
            exp_vertex_size32
        },
        {
            (BYTE*)vertices33,
            NULL,
            NULL,
            num_vertices33,
            num_faces33,
            vertex_size33,
            options,
            options,
            declaration_ptc_short4n,
            declaration_ptc_float4,
            (BYTE*)exp_vertices33,
            exp_vertex_size33
        },
        {
            (BYTE*)vertices34,
            NULL,
            NULL,
            num_vertices34,
            num_faces34,
            vertex_size34,
            options,
            options,
            declaration_ptc_float16_2,
            declaration_ptc_float4,
            (BYTE*)exp_vertices34,
            exp_vertex_size34
        },
        {
            (BYTE*)vertices35,
            NULL,
            NULL,
            num_vertices35,
            num_faces35,
            vertex_size35,
            options,
            options,
            declaration_ptc_float16_4,
            declaration_ptc_float4,
            (BYTE*)exp_vertices35,
            exp_vertex_size35
        },
        {
            (BYTE*)vertices36,
            NULL,
            NULL,
            num_vertices36,
            num_faces36,
            vertex_size36,
            options,
            clone_options36,
            declaration_pn,
            declaration_pn,
            (BYTE*)vertices36,
            vertex_size36
        },
    };

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        goto cleanup;
    }

    for (i = 0; i < ARRAY_SIZE(tc); i++)
    {
        UINT j;
        D3DVERTEXELEMENT9 new_declaration[MAX_FVF_DECL_SIZE];
        UINT exp_new_decl_length, new_decl_length;
        UINT exp_new_decl_size, new_decl_size;

        hr = init_test_mesh(tc[i].num_faces, tc[i].num_vertices,
                              tc[i].create_options,
                              tc[i].declaration,
                              test_context->device, &mesh,
                              tc[i].vertices, tc[i].vertex_size,
                              tc[i].indices, tc[i].attributes);
        if (FAILED(hr))
        {
            skip("Couldn't initialize test mesh %d. Got %lx expected D3D_OK\n", i, hr);
            goto cleanup;
        }

        hr = mesh->lpVtbl->CloneMesh(mesh, tc[i].clone_options, tc[i].new_declaration,
                                     test_context->device, &mesh_clone);
        ok(hr == D3D_OK, "Test %u, got unexpected hr %#lx.\n", i, hr);

        hr = mesh_clone->lpVtbl->GetDeclaration(mesh_clone, new_declaration);
        ok(hr == D3D_OK, "Test %u, got unexpected hr %#lx.\n", i, hr);
        /* Check declaration elements */
        for (j = 0; tc[i].new_declaration[j].Stream != 0xFF; j++)
        {
            ok(memcmp(&tc[i].new_declaration[j], &new_declaration[j], sizeof(*new_declaration)) == 0,
               "Test case %d failed. Declaration element %d did not match.\n", i, j);
        }

        /* Check declaration length */
        exp_new_decl_length = D3DXGetDeclLength(tc[i].new_declaration);
        new_decl_length = D3DXGetDeclLength(new_declaration);
        ok(new_decl_length == exp_new_decl_length,
           "Test case %d failed. Got new declaration length %d, expected %d\n",
           i, new_decl_length, exp_new_decl_length);

        /* Check declaration size */
        exp_new_decl_size = D3DXGetDeclVertexSize(tc[i].new_declaration, 0);
        new_decl_size = D3DXGetDeclVertexSize(new_declaration, 0);
        ok(new_decl_size == exp_new_decl_size,
           "Test case %d failed. Got new declaration size %d, expected %d\n",
           i, new_decl_size, exp_new_decl_size);

        /* Check vertex data in cloned mesh */
        hr = mesh_clone->lpVtbl->LockVertexBuffer(mesh_clone, 0, (void**)&vertices);
        if (FAILED(hr))
        {
            skip("Couldn't lock cloned vertex buffer.\n");
            goto cleanup;
        }
        for (j = 0; j < tc[i].num_vertices; j++)
        {
            UINT index = tc[i].exp_vertex_size * j;
            check_vertex_components(__LINE__, i, j, &vertices[index], &tc[i].exp_vertices[index], tc[i].new_declaration);
        }
        hr = mesh_clone->lpVtbl->UnlockVertexBuffer(mesh_clone);
        if (FAILED(hr))
        {
            skip("Couldn't unlock vertex buffer.\n");
            goto cleanup;
        }
        vertices = NULL;
        mesh->lpVtbl->Release(mesh);
        mesh = NULL;
        mesh_clone->lpVtbl->Release(mesh_clone);
        mesh_clone = NULL;
    }

    /* The following test shows that it is not possible to share a vertex buffer
     * with D3DXMESH_VB_SHARE and change the vertex declaration at the same
     * time. It reuses the test data from test 2.
     */
    hr = init_test_mesh(tc[2].num_faces, tc[2].num_vertices,
                        tc[2].create_options,
                        tc[2].declaration,
                        test_context->device, &mesh,
                        tc[2].vertices, tc[2].vertex_size,
                        tc[2].indices, tc[2].attributes);
    if (FAILED(hr))
    {
        skip("Couldn't initialize test mesh for D3DXMESH_VB_SHARE case."
             " Got %lx expected D3D_OK\n", hr);
        goto cleanup;
    }

    hr = mesh->lpVtbl->CloneMesh(mesh, tc[2].create_options | D3DXMESH_VB_SHARE,
                                 tc[2].new_declaration, test_context->device,
                                 &mesh_clone);
    ok(hr == D3DERR_INVALIDCALL, "CloneMesh D3DXMESH_VB_SHARE with new"
       " declaration. Got %lx, expected D3DERR_INVALIDCALL\n",
       hr);
    mesh->lpVtbl->Release(mesh);
    mesh = NULL;
    mesh_clone = NULL;

cleanup:
    if (vertices) mesh->lpVtbl->UnlockVertexBuffer(mesh);
    if (mesh) mesh->lpVtbl->Release(mesh);
    if (mesh_clone) mesh_clone->lpVtbl->Release(mesh_clone);
    free_test_context(test_context);
}

static void test_valid_mesh(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    UINT i;
    const DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    const D3DVERTEXELEMENT9 declaration[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };
    const unsigned int VERTS_PER_FACE = 3;
    /* mesh0 (one face)
     *
     * 0--1
     * | /
     * |/
     * 2
     */
    const D3DXVECTOR3 vertices0[] =
    {
        { 0.0f,  3.0f,  0.f},
        { 2.0f,  3.0f,  0.f},
        { 0.0f,  0.0f,  0.f},
    };
    const DWORD indices0[] = {0, 1, 2};
    const unsigned int num_vertices0 = ARRAY_SIZE(vertices0);
    const unsigned int num_faces0 = ARRAY_SIZE(indices0) / VERTS_PER_FACE;
    const DWORD adjacency0[] = {-1, -1, -1};
    const HRESULT exp_hr0 = D3D_OK;
    /* mesh1 (Simple bow-tie)
     *
     * 0--1 1--3
     * | /   \ |
     * |/     \|
     * 2       4
     */
    const D3DXVECTOR3 vertices1[] =
    {
        { 0.0f,  3.0f,  0.f},
        { 2.0f,  3.0f,  0.f},
        { 0.0f,  0.0f,  0.f},

        { 4.0f,  3.0f,  0.f},
        { 4.0f,  0.0f,  0.f},
    };
    const DWORD indices1[] = {0, 1, 2, 1, 3, 4};
    const unsigned int num_vertices1 = ARRAY_SIZE(vertices1);
    const unsigned int num_faces1 = ARRAY_SIZE(indices1) / VERTS_PER_FACE;
    const DWORD adjacency1[] = {-1, -1, -1, -1, -1, -1};
    const HRESULT exp_hr1 = D3DXERR_INVALIDMESH;
    /* Common mesh data */
    ID3DXMesh *mesh = NULL;
    UINT vertex_size = sizeof(D3DXVECTOR3);
    ID3DXBuffer *errors_and_warnings = NULL;
    struct
    {
        const D3DXVECTOR3 *vertices;
        const DWORD *indices;
        const UINT num_vertices;
        const UINT num_faces;
        const DWORD *adjacency;
        const HRESULT exp_hr;
    }
    tc[] =
    {
        {
            vertices0,
            indices0,
            num_vertices0,
            num_faces0,
            adjacency0,
            exp_hr0,
        },
        {
            vertices1,
            indices1,
            num_vertices1,
            num_faces1,
            adjacency1,
            exp_hr1,
        },
    };

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        goto cleanup;
    }

    for (i = 0; i < ARRAY_SIZE(tc); i++)
    {
        hr = init_test_mesh(tc[i].num_faces, tc[i].num_vertices,
                            options, declaration,
                            test_context->device, &mesh,
                            tc[i].vertices, vertex_size,
                            tc[i].indices, NULL);
        if (FAILED(hr))
        {
            skip("Couldn't initialize test mesh %d. Got %lx expected D3D_OK\n", i, hr);
            goto cleanup;
        }

        hr = D3DXValidMesh(mesh, tc[i].adjacency, &errors_and_warnings);
        todo_wine ok(hr == tc[i].exp_hr, "Test %u, got unexpected hr %#lx, expected %#lx.\n", i, hr, tc[i].exp_hr);

        /* Note errors_and_warnings is deliberately not checked because that
         * would require copying wast amounts of the text output. */
        if (errors_and_warnings)
        {
            ID3DXBuffer_Release(errors_and_warnings);
            errors_and_warnings = NULL;
        }
        mesh->lpVtbl->Release(mesh);
        mesh = NULL;
    }

cleanup:
    if (mesh) mesh->lpVtbl->Release(mesh);
    free_test_context(test_context);
}

static void test_optimize_faces(void)
{
    HRESULT hr;
    UINT i;
    DWORD smallest_face_remap;
    /* mesh0
     *
     * 0--1
     * | /
     * |/
     * 2
     */
    const DWORD indices0[] = {0, 1, 2};
    const UINT num_faces0 = 1;
    const UINT num_vertices0 = 3;
    const DWORD exp_face_remap0[] = {0};
    /* mesh1
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const DWORD indices1[] = {0, 1, 2, 3, 4, 5};
    const UINT num_faces1 = 2;
    const UINT num_vertices1 = 6;
    const DWORD exp_face_remap1[] = {1, 0};
    /* mesh2
     *
     * 0--1
     * | /|
     * |/ |
     * 2--3
     */
    const DWORD indices2[] = {0, 1, 2, 1, 3, 2};
    const UINT num_faces2 = 2;
    const UINT num_vertices2 = 4;
    const DWORD exp_face_remap2[] = {1, 0};
    /* mesh3
     *
     * 0--1
     * | /|
     * |/ |
     * 2--3
     * | /|
     * |/ |
     * 4--5
     */
    const DWORD indices3[] = {0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 4, 5};
    const UINT num_faces3 = 4;
    const UINT num_vertices3 = 6;
    const DWORD exp_face_remap3[] = {3, 2, 1, 0};
    /* mesh4
     *
     * 0--1
     * | /|
     * |/ |
     * 2--3
     * | /|
     * |/ |
     * 4--5
     */
    const WORD indices4[] = {0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 4, 5};
    const UINT num_faces4 = 4;
    const UINT num_vertices4 = 6;
    const DWORD exp_face_remap4[] = {3, 2, 1, 0};
    /* Test cases are stored in the tc array */
    struct
    {
        const VOID *indices;
        const UINT num_faces;
        const UINT num_vertices;
        const BOOL indices_are_32bit;
        const DWORD *exp_face_remap;
    }
    tc[] =
    {
        {
            indices0,
            num_faces0,
            num_vertices0,
            TRUE,
            exp_face_remap0
        },
        {
            indices1,
            num_faces1,
            num_vertices1,
            TRUE,
            exp_face_remap1
        },
        {
            indices2,
            num_faces2,
            num_vertices2,
            TRUE,
            exp_face_remap2
        },
        {
            indices3,
            num_faces3,
            num_vertices3,
            TRUE,
            exp_face_remap3
        },
        {
            indices4,
            num_faces4,
            num_vertices4,
            FALSE,
            exp_face_remap4
        },
    };

    /* Go through all test cases */
    for (i = 0; i < ARRAY_SIZE(tc); i++)
    {
        DWORD j;
        DWORD *face_remap;
        face_remap = calloc(tc[i].num_faces, sizeof(*face_remap));

        hr = D3DXOptimizeFaces(tc[i].indices, tc[i].num_faces,
                               tc[i].num_vertices, tc[i].indices_are_32bit,
                               face_remap);
        ok(hr == D3D_OK, "Test %u, got unexpected hr %#lx.\n", i, hr);

        /* Compare face remap with expected face remap */
        for (j = 0; j < tc[i].num_faces; j++)
        {
            ok(tc[i].exp_face_remap[j] == face_remap[j],
               "Test case %d: Got face %ld at %ld, expected %ld\n", i,
               face_remap[j], j, tc[i].exp_face_remap[j]);
        }

        free(face_remap);
    }

    /* face_remap must not be NULL */
    hr = D3DXOptimizeFaces(tc[0].indices, tc[0].num_faces,
                           tc[0].num_vertices, tc[0].indices_are_32bit,
                           NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    /* Number of faces must be smaller than 2^15 */
    hr = D3DXOptimizeFaces(tc[0].indices, 2 << 15,
                           tc[0].num_vertices, FALSE,
                           &smallest_face_remap);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
}

static void test_optimize_vertices(void)
{
    static const WORD indices_16bit[] = {0, 1, 2};
    static const DWORD indices[] = {0, 1, 2};
    DWORD vertex_remap[3];
    unsigned int i;
    HRESULT hr;

    hr = D3DXOptimizeVertices(indices, 1, 3, TRUE, vertex_remap);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 3; ++i)
        ok(vertex_remap[i] == i, "Unexpected vertex remap %u -> %lu.\n", i, vertex_remap[i]);

    hr = D3DXOptimizeVertices(indices_16bit, 1, 3, FALSE, vertex_remap);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 3; ++i)
        ok(vertex_remap[i] == i, "Unexpected vertex remap %u -> %lu.\n", i, vertex_remap[i]);

    hr = D3DXOptimizeVertices(indices, 0, 3, TRUE, vertex_remap);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DXOptimizeVertices(indices, 1, 0, TRUE, vertex_remap);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DXOptimizeVertices(NULL, 1, 3, TRUE, vertex_remap);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DXOptimizeVertices(indices, 1, 3, TRUE, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
}

static HRESULT clear_normals(ID3DXMesh *mesh)
{
    HRESULT hr;
    BYTE *vertices;
    size_t normal_size;
    DWORD i, num_vertices, vertex_stride;
    const D3DXVECTOR4 normal = {NAN, NAN, NAN, NAN};
    D3DVERTEXELEMENT9 *normal_declaration = NULL;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE] = {D3DDECL_END()};

    if (FAILED(hr = mesh->lpVtbl->GetDeclaration(mesh, declaration)))
        return hr;

    for (i = 0; declaration[i].Stream != 0xff; i++)
    {
        if (declaration[i].Usage == D3DDECLUSAGE_NORMAL && !declaration[i].UsageIndex)
        {
            normal_declaration = &declaration[i];
            break;
        }
    }

    if (!normal_declaration)
        return D3DERR_INVALIDCALL;

    if (normal_declaration->Type == D3DDECLTYPE_FLOAT3)
    {
        normal_size = sizeof(D3DXVECTOR3);
    }
    else if (normal_declaration->Type == D3DDECLTYPE_FLOAT4)
    {
        normal_size = sizeof(D3DXVECTOR4);
    }
    else
    {
        trace("Cannot clear normals\n");
        return E_NOTIMPL;
    }

    num_vertices = mesh->lpVtbl->GetNumVertices(mesh);
    vertex_stride = mesh->lpVtbl->GetNumBytesPerVertex(mesh);

    if (FAILED(hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, (void **)&vertices)))
        return hr;

    vertices += normal_declaration->Offset;

    for (i = 0; i < num_vertices; i++, vertices += vertex_stride)
        memcpy(vertices, &normal, normal_size);

    return mesh->lpVtbl->UnlockVertexBuffer(mesh);
}

static void compare_normals(unsigned int line, const char *test_name,
        ID3DXMesh *mesh, const D3DXVECTOR3 *normals, unsigned int num_normals)
{
    unsigned int i;
    BYTE *vertices;
    DWORD num_vertices, vertex_stride;
    D3DVERTEXELEMENT9 *normal_declaration = NULL;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE] = {D3DDECL_END()};

    if (FAILED(mesh->lpVtbl->GetDeclaration(mesh, declaration)))
    {
        ok_(__FILE__, line)(0, "%s: Failed to get declaration\n", test_name);
        return;
    }

    for (i = 0; declaration[i].Stream != 0xff; i++)
    {
        if (declaration[i].Usage == D3DDECLUSAGE_NORMAL && !declaration[i].UsageIndex)
        {
            normal_declaration = &declaration[i];
            break;
        }
    }

    if (!normal_declaration)
    {
        ok_(__FILE__, line)(0, "%s: Mesh has no normals\n", test_name);
        return;
    }

    if (normal_declaration->Type != D3DDECLTYPE_FLOAT3 && normal_declaration->Type != D3DDECLTYPE_FLOAT4)
    {
        ok_(__FILE__, line)(0, "%s: Mesh has invalid normals type\n", test_name);
        return;
    }

    num_vertices = mesh->lpVtbl->GetNumVertices(mesh);
    vertex_stride = mesh->lpVtbl->GetNumBytesPerVertex(mesh);

    ok_(__FILE__, line)(num_vertices == num_normals, "%s: Expected %u vertices, got %lu\n", test_name,
            num_normals, num_vertices);

    if (FAILED(mesh->lpVtbl->LockVertexBuffer(mesh, 0, (void **)&vertices)))
    {
        ok_(__FILE__, line)(0, "%s: Failed to compare normals\n", test_name);
        return;
    }

    vertices += normal_declaration->Offset;

    for (i = 0; i < min(num_vertices, num_normals); i++, vertices += vertex_stride)
    {
        if (normal_declaration->Type == D3DDECLTYPE_FLOAT3)
        {
            const D3DXVECTOR3 *n = (D3DXVECTOR3 *)vertices;
            ok_(__FILE__, line)(compare_vec3(*n, normals[i]),
                    "%s: normal %2u, expected (%f, %f, %f), got (%f, %f, %f)\n",
                    test_name, i, normals[i].x, normals[i].y, normals[i].z, n->x, n->y, n->z);
        }
        else
        {
            const D3DXVECTOR4 *n = (D3DXVECTOR4 *)vertices;
            const D3DXVECTOR4 normal = {normals[i].x, normals[i].y, normals[i].z, 1.0f};
            ok_(__FILE__, line)(compare_vec4(*n, normal),
                    "%s: normal %2u, expected (%f, %f, %f, %f), got (%f, %f, %f, %f)\n",
                    test_name, i, normals[i].x, normals[i].y, normals[i].z, 1.0f,
                    n->x, n->y, n->z, n->w);
        }
    }

    mesh->lpVtbl->UnlockVertexBuffer(mesh);
}

static HRESULT compute_normals_D3DXComputeNormals(ID3DXMesh *mesh, const DWORD *adjacency)
{
    return D3DXComputeNormals((ID3DXBaseMesh *)mesh, adjacency);
}

static HRESULT compute_normals_D3DXComputeTangentFrameEx(ID3DXMesh *mesh, const DWORD *adjacency)
{
    return D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS,
            adjacency, -1.01f, -0.01f, -1.01f, NULL, NULL);
}

static void test_compute_normals(void)
{
    HRESULT hr;
    ULONG refcount;
    ID3DXMesh *mesh, *cloned_mesh;
    ID3DXBuffer *adjacency;
    IDirect3DDevice9 *device;
    struct test_context *test_context;
    unsigned int i;

    static const struct compute_normals_func
    {
        const char *name;
        HRESULT (*apply)(ID3DXMesh *mesh, const DWORD *adjacency);
    }
    compute_normals_funcs[] =
    {
        {"D3DXComputeNormals",        compute_normals_D3DXComputeNormals       },
        {"D3DXComputeTangentFrameEx", compute_normals_D3DXComputeTangentFrameEx}
    };

    static const D3DXVECTOR3 box_normals[24] =
    {
        {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f},
        { 1.0f, 0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f},
        { 0.0f,-1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f},
        { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f},
        { 0.0f, 0.0f,-1.0f}, { 0.0f, 0.0f,-1.0f}, { 0.0f, 0.0f,-1.0f}, { 0.0f, 0.0f,-1.0f}
    };
    const float box_normal_component = 1.0f / sqrtf(3.0f);
    const D3DXVECTOR3 box_normals_adjacency[24] =
    {
        {-box_normal_component, -box_normal_component, -box_normal_component},
        {-box_normal_component, -box_normal_component,  box_normal_component},
        {-box_normal_component,  box_normal_component,  box_normal_component},
        {-box_normal_component,  box_normal_component, -box_normal_component},
        {-box_normal_component,  box_normal_component, -box_normal_component},
        {-box_normal_component,  box_normal_component,  box_normal_component},
        { box_normal_component,  box_normal_component,  box_normal_component},
        { box_normal_component,  box_normal_component, -box_normal_component},
        { box_normal_component,  box_normal_component, -box_normal_component},
        { box_normal_component,  box_normal_component,  box_normal_component},
        { box_normal_component, -box_normal_component,  box_normal_component},
        { box_normal_component, -box_normal_component, -box_normal_component},
        {-box_normal_component, -box_normal_component,  box_normal_component},
        {-box_normal_component, -box_normal_component, -box_normal_component},
        { box_normal_component, -box_normal_component, -box_normal_component},
        { box_normal_component, -box_normal_component,  box_normal_component},
        {-box_normal_component, -box_normal_component,  box_normal_component},
        { box_normal_component, -box_normal_component,  box_normal_component},
        { box_normal_component,  box_normal_component,  box_normal_component},
        {-box_normal_component,  box_normal_component,  box_normal_component},
        {-box_normal_component, -box_normal_component, -box_normal_component},
        {-box_normal_component,  box_normal_component, -box_normal_component},
        { box_normal_component,  box_normal_component, -box_normal_component},
        { box_normal_component, -box_normal_component, -box_normal_component}
    };
    static const D3DXVECTOR3 box_normals_adjacency_area[24] =
    {
        {-0.666667f, -0.333333f, -0.666667f}, {-0.333333f, -0.666667f,  0.666667f},
        {-0.816496f,  0.408248f,  0.408248f}, {-0.408248f,  0.816496f, -0.408248f},
        {-0.408248f,  0.816496f, -0.408248f}, {-0.816496f,  0.408248f,  0.408248f},
        { 0.333333f,  0.666667f,  0.666667f}, { 0.666667f,  0.333333f, -0.666667f},
        { 0.666667f,  0.333333f, -0.666667f}, { 0.333333f,  0.666667f,  0.666667f},
        { 0.816496f, -0.408248f,  0.408248f}, { 0.408248f, -0.816496f, -0.408248f},
        {-0.333333f, -0.666667f,  0.666667f}, {-0.666667f, -0.333333f, -0.666667f},
        { 0.408248f, -0.816496f, -0.408248f}, { 0.816496f, -0.408248f,  0.408248f},
        {-0.333333f, -0.666667f,  0.666667f}, { 0.816497f, -0.408248f,  0.408248f},
        { 0.333333f,  0.666667f,  0.666667f}, {-0.816497f,  0.408248f,  0.408248f},
        {-0.666667f, -0.333333f, -0.666667f}, {-0.408248f,  0.816497f, -0.408248f},
        { 0.666667f,  0.333333f, -0.666667f}, { 0.408248f, -0.816496f, -0.408248f}
    };
    static const D3DXVECTOR3 box_normals_position1f[24] = {{0}};
    static const D3DXVECTOR3 box_normals_position2f[24] =
    {
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f,  1.0f},
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f,  1.0f},
        {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f,  1.0f},
        {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}
    };

    static const D3DXVECTOR3 sphere_normals[22] =
    {
        { 0.000000f, -0.000000f,  1.000000f}, { 0.000000f,  0.582244f,  0.813014f},
        { 0.582244f, -0.000000f,  0.813014f}, {-0.000000f, -0.582244f,  0.813014f},
        {-0.582244f,  0.000000f,  0.813014f}, {-0.000000f,  0.890608f,  0.454772f},
        { 0.890608f,  0.000000f,  0.454772f}, { 0.000000f, -0.890608f,  0.454772f},
        {-0.890608f, -0.000000f,  0.454772f}, { 0.000000f,  1.000000f, -0.000000f},
        { 1.000000f, -0.000000f, -0.000000f}, {-0.000000f, -1.000000f, -0.000000f},
        {-1.000000f,  0.000000f, -0.000000f}, { 0.000000f,  0.890608f, -0.454773f},
        { 0.890608f, -0.000000f, -0.454772f}, {-0.000000f, -0.890608f, -0.454773f},
        {-0.890608f,  0.000000f, -0.454773f}, { 0.000000f,  0.582244f, -0.813015f},
        { 0.582244f, -0.000000f, -0.813015f}, { 0.000000f, -0.582244f, -0.813015f},
        {-0.582243f,  0.000000f, -0.813015f}, { 0.000000f,  0.000000f, -1.000000f}
    };
    static const D3DXVECTOR3 sphere_normals_area[22] =
    {
        { 0.000000f, -0.000000f,  1.000000f}, {-0.215311f,  0.554931f,  0.803550f},
        { 0.554931f,  0.215311f,  0.803550f}, { 0.215311f, -0.554931f,  0.803550f},
        {-0.554931f, -0.215311f,  0.803550f}, {-0.126638f,  0.872121f,  0.472618f},
        { 0.872121f,  0.126638f,  0.472618f}, { 0.126638f, -0.872121f,  0.472618f},
        {-0.872121f, -0.126637f,  0.472618f}, { 0.000000f,  1.000000f, -0.000000f},
        { 1.000000f, -0.000000f, -0.000000f}, {-0.000000f, -1.000000f, -0.000000f},
        {-1.000000f,  0.000000f, -0.000000f}, { 0.126638f,  0.872121f, -0.472618f},
        { 0.872121f, -0.126638f, -0.472618f}, {-0.126638f, -0.872121f, -0.472618f},
        {-0.872121f,  0.126638f, -0.472618f}, { 0.215311f,  0.554931f, -0.803550f},
        { 0.554931f, -0.215311f, -0.803550f}, {-0.215311f, -0.554931f, -0.803550f},
        {-0.554931f,  0.215311f, -0.803550f}, { 0.000000f,  0.000000f, -1.000000f}
    };
    static const D3DXVECTOR3 sphere_normals_equal[22] =
    {
        { 0.000000f, -0.000000f,  1.000000f}, {-0.134974f,  0.522078f,  0.842150f},
        { 0.522078f,  0.134974f,  0.842150f}, { 0.134974f, -0.522078f,  0.842150f},
        {-0.522078f, -0.134974f,  0.842150f}, {-0.026367f,  0.857121f,  0.514440f},
        { 0.857121f,  0.026367f,  0.514440f}, { 0.026367f, -0.857121f,  0.514440f},
        {-0.857121f, -0.026367f,  0.514440f}, { 0.000000f,  1.000000f, -0.000000f},
        { 1.000000f, -0.000000f, -0.000000f}, {-0.000000f, -1.000000f, -0.000000f},
        {-1.000000f,  0.000000f, -0.000000f}, { 0.026367f,  0.857121f, -0.514440f},
        { 0.857121f, -0.026367f, -0.514440f}, {-0.026367f, -0.857121f, -0.514440f},
        {-0.857121f,  0.026367f, -0.514440f}, { 0.134975f,  0.522078f, -0.842150f},
        { 0.522078f, -0.134975f, -0.842150f}, {-0.134974f, -0.522078f, -0.842150f},
        {-0.522078f,  0.134974f, -0.842150f}, { 0.000000f,  0.000000f, -1.000000f}
    };

    static const D3DVERTEXELEMENT9 position3f_normal1f_declaration[] =
    {
        {0, 0,                   D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, sizeof(D3DXVECTOR3), D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 position3f_normal2f_declaration[] =
    {
        {0, 0,                   D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, sizeof(D3DXVECTOR3), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 normal4f_position3f_declaration[] =
    {
        {0, 0,                   D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        {0, sizeof(D3DXVECTOR4), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 position1f_normal3f_declaration[] =
    {
        {0, 0,                   D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, sizeof(float),       D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 position2f_normal3f_declaration[] =
    {
        {0, 0,                   D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, sizeof(D3DXVECTOR2), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 position4f_normal3f_declaration[] =
    {
        {0, 0,                   D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, sizeof(D3DXVECTOR4), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        D3DDECL_END()
    };

    for (i = 0; i < ARRAY_SIZE(compute_normals_funcs); i++)
    {
        hr = compute_normals_funcs[i].apply(NULL, NULL);
        ok(hr == D3DERR_INVALIDCALL, "%s returned %#lx, expected D3DERR_INVALIDCALL\n", compute_normals_funcs[i].name, hr);
    }

    if (!(test_context = new_test_context()))
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateBox(device, 1.0f, 1.0f, 1.0f, &mesh, &adjacency);
    ok(SUCCEEDED(hr), "D3DXCreateBox failed %#lx\n", hr);

    /* Check wrong input */
    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE, NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "D3DXComputeTangentFrameEx returned %#lx, expected D3DERR_INVALIDCALL\n", hr);

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DDECLUSAGE_NORMAL, 0,
            D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_BY_AREA | D3DXTANGENT_WEIGHT_EQUAL,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXComputeTangentFrameEx returned %#lx, expected D3DERR_INVALIDCALL\n", hr);

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, 0, NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "D3DXComputeTangentFrameEx returned %#lx, expected D3DERR_INVALIDCALL\n", hr);

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 1, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXComputeTangentFrameEx returned %#lx, expected D3DERR_INVALIDCALL\n", hr);

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DX_DEFAULT, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXComputeTangentFrameEx returned %#lx, expected D3DERR_INVALIDCALL\n", hr);

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_CALCULATE_NORMALS,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "D3DXComputeTangentFrameEx returned %#lx, expected D3DERR_INVALIDCALL\n", hr);

    for (i = 0; i < ARRAY_SIZE(compute_normals_funcs); i++)
    {
        const struct compute_normals_func *func = &compute_normals_funcs[i];

        /* Mesh without normals */
        hr = mesh->lpVtbl->CloneMeshFVF(mesh, 0, D3DFVF_XYZ, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMeshFVF failed %#lx\n", hr);

        hr = func->apply(cloned_mesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "%s returned %#lx, expected D3DERR_INVALIDCALL\n", func->name, hr);

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);

        /* Mesh without positions */
        hr = mesh->lpVtbl->CloneMeshFVF(mesh, 0, D3DFVF_NORMAL, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMeshFVF failed %#lx\n", hr);

        hr = func->apply(cloned_mesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "%s returned %#lx, expected D3DERR_INVALIDCALL\n", func->name, hr);

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);

        /* Mesh with D3DDECLTYPE_FLOAT1 normals */
        hr = mesh->lpVtbl->CloneMesh(mesh, 0, position3f_normal1f_declaration, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMesh failed %#lx\n", hr);

        hr = func->apply(cloned_mesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "%s returned %#lx, expected D3DERR_INVALIDCALL\n", func->name, hr);

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);

        /* Mesh with D3DDECLTYPE_FLOAT2 normals */
        hr = mesh->lpVtbl->CloneMesh(mesh, 0, position3f_normal2f_declaration, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMesh failed %#lx\n", hr);

        hr = func->apply(cloned_mesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "%s returned %#lx, expected D3DERR_INVALIDCALL\n", func->name, hr);

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);

        /* Mesh without adjacency data */
        hr = clear_normals(mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(mesh, NULL);
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, mesh, box_normals, ARRAY_SIZE(box_normals));

        /* Mesh with adjacency data */
        hr = clear_normals(mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(mesh, ID3DXBuffer_GetBufferPointer(adjacency));
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, mesh, box_normals_adjacency, ARRAY_SIZE(box_normals_adjacency));

        /* Mesh with custom vertex format, D3DDECLTYPE_FLOAT4 normals and adjacency */
        hr = mesh->lpVtbl->CloneMesh(mesh, 0, normal4f_position3f_declaration, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMesh failed %#lx\n", hr);

        hr = clear_normals(cloned_mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(cloned_mesh, ID3DXBuffer_GetBufferPointer(adjacency));
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, cloned_mesh, box_normals_adjacency, ARRAY_SIZE(box_normals_adjacency));

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);

        /* Mesh with D3DDECLTYPE_FLOAT1 positions and D3DDECLTYPE_FLOAT3 normals */
        hr = mesh->lpVtbl->CloneMesh(mesh, 0, position1f_normal3f_declaration, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMesh failed %#lx\n", hr);

        hr = clear_normals(cloned_mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(cloned_mesh, ID3DXBuffer_GetBufferPointer(adjacency));
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, cloned_mesh, box_normals_position1f, ARRAY_SIZE(box_normals_position1f));

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);

        /* Mesh with D3DDECLTYPE_FLOAT2 positions and D3DDECLTYPE_FLOAT3 normals */
        hr = mesh->lpVtbl->CloneMesh(mesh, 0, position2f_normal3f_declaration, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMesh failed %#lx\n", hr);

        hr = clear_normals(cloned_mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(cloned_mesh, ID3DXBuffer_GetBufferPointer(adjacency));
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, cloned_mesh, box_normals_position2f, ARRAY_SIZE(box_normals_position2f));

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);

        /* Mesh with D3DDECLTYPE_FLOAT4 positions and D3DDECLTYPE_FLOAT3 normals */
        hr = mesh->lpVtbl->CloneMesh(mesh, 0, position4f_normal3f_declaration, device, &cloned_mesh);
        ok(SUCCEEDED(hr), "CloneMesh failed %#lx\n", hr);

        hr = clear_normals(cloned_mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(cloned_mesh, ID3DXBuffer_GetBufferPointer(adjacency));
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, cloned_mesh, box_normals_adjacency, ARRAY_SIZE(box_normals_adjacency));

        refcount = cloned_mesh->lpVtbl->Release(cloned_mesh);
        ok(!refcount, "Mesh has %lu references left\n", refcount);
    }

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_BY_AREA,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, box_normals, ARRAY_SIZE(box_normals));

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_BY_AREA,
            ID3DXBuffer_GetBufferPointer(adjacency), -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, box_normals_adjacency_area, ARRAY_SIZE(box_normals_adjacency_area));

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_EQUAL,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, box_normals, ARRAY_SIZE(box_normals));

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_EQUAL,
            ID3DXBuffer_GetBufferPointer(adjacency), -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, box_normals_adjacency_area, ARRAY_SIZE(box_normals_adjacency_area));

    refcount = mesh->lpVtbl->Release(mesh);
    ok(!refcount, "Mesh has %lu references left\n", refcount);
    refcount = ID3DXBuffer_Release(adjacency);
    ok(!refcount, "Buffer has %lu references left\n", refcount);

    hr = D3DXCreateSphere(device, 1.0f, 4, 6, &mesh, &adjacency);
    ok(SUCCEEDED(hr), "D3DXCreateSphere failed %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(compute_normals_funcs); i++)
    {
        const struct compute_normals_func *func = &compute_normals_funcs[i];

        /* Sphere without adjacency data */
        hr = clear_normals(mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(mesh, NULL);
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, mesh, sphere_normals, ARRAY_SIZE(sphere_normals));

        /* Sphere with adjacency data */
        hr = clear_normals(mesh);
        ok(SUCCEEDED(hr), "Failed to clear normals, returned %#lx\n", hr);

        hr = func->apply(mesh, ID3DXBuffer_GetBufferPointer(adjacency));
        ok(hr == D3D_OK, "%s returned %#lx, expected D3D_OK\n", func->name, hr);

        compare_normals(__LINE__, func->name, mesh, sphere_normals, ARRAY_SIZE(sphere_normals));
    }

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_BY_AREA,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, sphere_normals_area, ARRAY_SIZE(sphere_normals_area));

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_BY_AREA,
            ID3DXBuffer_GetBufferPointer(adjacency), -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, sphere_normals_area, ARRAY_SIZE(sphere_normals_area));

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_EQUAL,
            NULL, -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, sphere_normals_equal, ARRAY_SIZE(sphere_normals_equal));

    hr = D3DXComputeTangentFrameEx(mesh, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DX_DEFAULT, 0,
            D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS | D3DXTANGENT_WEIGHT_EQUAL,
            ID3DXBuffer_GetBufferPointer(adjacency), -1.01f, -0.01f, -1.01f, NULL, NULL);
    ok(hr == D3D_OK, "D3DXComputeTangentFrameEx returned %#lx, expected D3D_OK\n", hr);

    compare_normals(__LINE__, "D3DXComputeTangentFrameEx", mesh, sphere_normals_equal, ARRAY_SIZE(sphere_normals_equal));

    refcount = mesh->lpVtbl->Release(mesh);
    ok(!refcount, "Mesh has %lu references left\n", refcount);
    refcount = ID3DXBuffer_Release(adjacency);
    ok(!refcount, "Buffer has %lu references left\n", refcount);

    free_test_context(test_context);
}

static void D3DXCreateAnimationControllerTest(void)
{
    HRESULT hr;
    ID3DXAnimationController *animation;
    UINT value;

    hr = D3DXCreateAnimationController(0, 0, 0, 0, NULL);
    ok(hr == D3D_OK, "Got unexpected hr returned %#lx.\n", hr);

    animation = (void*)0xdeadbeef;
    hr = D3DXCreateAnimationController(0, 1, 1, 1, &animation);
    ok(hr == D3D_OK, "Got unexpected hr returned %#lx.\n", hr);
    ok(animation == (void*)0xdeadbeef, "Got unexpected animation %p.\n", animation);

    animation = (void*)0xdeadbeef;
    hr = D3DXCreateAnimationController(1, 0, 1, 1, &animation);
    ok(hr == D3D_OK, "Got unexpected hr returned %#lx.\n", hr);
    ok(animation == (void*)0xdeadbeef, "Got unexpected animation %p.\n", animation);

    animation = (void*)0xdeadbeef;
    hr = D3DXCreateAnimationController(1, 1, 0, 1, &animation);
    ok(hr == D3D_OK, "Got unexpected hr returned %#lx.\n", hr);
    ok(animation == (void*)0xdeadbeef, "Got unexpected animation %p.\n", animation);

    animation = (void*)0xdeadbeef;
    hr = D3DXCreateAnimationController(1, 1, 1, 0, &animation);
    ok(hr == D3D_OK, "Got unexpected hr returned %#lx.\n", hr);
    ok(animation == (void*)0xdeadbeef, "Got unexpected animation %p.\n", animation);

    hr = D3DXCreateAnimationController(1, 1, 1, 1, &animation);
    ok(hr == D3D_OK, "Got unexpected hr returned %#lx.\n", hr);

    value = animation->lpVtbl->GetMaxNumAnimationOutputs(animation);
    ok(value == 1, "Got unexpected value %u.\n", value);

    value = animation->lpVtbl->GetMaxNumAnimationSets(animation);
    ok(value == 1, "Got unexpected value %u.\n", value);

    value = animation->lpVtbl->GetMaxNumTracks(animation);
    ok(value == 1, "Got unexpected value %u.\n", value);

    value = animation->lpVtbl->GetMaxNumEvents(animation);
    ok(value == 1, "Got unexpected value %u.\n", value);

    animation->lpVtbl->Release(animation);

    hr = D3DXCreateAnimationController(100, 101, 102, 103, &animation);
    ok(hr == D3D_OK, "Got unexpected hr returned %#lx.\n", hr);

    value = animation->lpVtbl->GetMaxNumAnimationOutputs(animation);
    ok(value == 100, "Got unexpected value %u.\n", value);

    value = animation->lpVtbl->GetMaxNumAnimationSets(animation);
    ok(value == 101, "Got unexpected value %u.\n", value);

    value = animation->lpVtbl->GetMaxNumTracks(animation);
    ok(value == 102, "Got unexpected value %u.\n", value);

    value = animation->lpVtbl->GetMaxNumEvents(animation);
    ok(value == 103, "Got unexpected value %u.\n", value);

    animation->lpVtbl->Release(animation);
}

static void D3DXCreateKeyframedAnimationSetTest(void)
{
    ID3DXKeyframedAnimationSet *set;
    D3DXPLAYBACK_TYPE type;
    unsigned int count;
    const char *name;
    HRESULT hr;

    hr = D3DXCreateKeyframedAnimationSet("wine_bottle", 5.0, D3DXPLAY_LOOP, 0, 0, NULL, &set);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXCreateKeyframedAnimationSet("wine_bottle", 5.0, D3DXPLAY_LOOP, 10, 0, NULL, &set);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    name = set->lpVtbl->GetName(set);
    ok(!strcmp(name, "wine_bottle"), "Got unexpected name %s.\n", debugstr_a(name));

    type = set->lpVtbl->GetPlaybackType(set);
    ok(type == D3DXPLAY_LOOP, "Got unexpected value %u.\n", type);

    count = set->lpVtbl->GetNumAnimations(set);
    ok(!count, "Got unexpected value %u.\n", count);

    set->lpVtbl->Release(set);
}

static void test_D3DXFrameFind(void)
{
    static char n1[] = "name1";
    static char n2[] = "name2";
    static char n3[] = "name3";
    static char n4[] = "name4";
    static char n5[] = "name5";
    static char n6[] = "name6";
    static char N1[] = "Name1";
    D3DXFRAME root, sibling, sibling2, child, *ret;
    D3DXFRAME child2, child3;

    ret = D3DXFrameFind(NULL, NULL);
    ok(ret == NULL, "Unexpected frame, %p.\n", ret);

    ret = D3DXFrameFind(NULL, "test");
    ok(ret == NULL, "Unexpected frame, %p.\n", ret);

    memset(&root, 0, sizeof(root));

    ret = D3DXFrameFind(&root, NULL);
    ok(ret == &root, "Unexpected frame, %p.\n", ret);

    root.Name = n1;
    ret = D3DXFrameFind(&root, NULL);
    ok(ret == NULL, "Unexpected frame, %p.\n", ret);

    ret = D3DXFrameFind(&root, n1);
    ok(ret == &root, "Unexpected frame, %p.\n", ret);

    ret = D3DXFrameFind(&root, N1);
    ok(ret == NULL, "Unexpected frame, %p.\n", ret);

    /* Test siblings order traversal. */
    memset(&sibling, 0, sizeof(sibling));
    sibling.Name = n2;
    root.pFrameSibling = &sibling;
    ret = D3DXFrameFind(&root, n2);
    ok(ret == &sibling, "Unexpected frame, %p.\n", ret);

    memset(&sibling2, 0, sizeof(sibling2));
    sibling2.Name = n2;
    sibling.pFrameSibling = &sibling2;
    ret = D3DXFrameFind(&root, n2);
    ok(ret == &sibling, "Unexpected frame, %p.\n", ret);

    sibling2.Name = n3;
    ret = D3DXFrameFind(&root, n3);
    ok(ret == &sibling2, "Unexpected frame, %p.\n", ret);

    /* Siblings first. */
    memset(&child, 0, sizeof(child));
    child.Name = n2;
    root.pFrameFirstChild = &child;
    ret = D3DXFrameFind(&root, n2);
    ok(ret == &sibling, "Unexpected frame, %p.\n", ret);

    child.Name = n4;
    ret = D3DXFrameFind(&root, n4);
    ok(ret == &child, "Unexpected frame, %p.\n", ret);

    /* Link a grandchild and another one for sibling. */
    memset(&child2, 0, sizeof(child2));
    memset(&child3, 0, sizeof(child3));
    child2.Name = child3.Name = n5;
    sibling.pFrameFirstChild = &child2;
    child.pFrameFirstChild = &child3;
    ret = D3DXFrameFind(&root, n5);
    ok(ret == &child2, "Unexpected frame, %p.\n", ret);

    child3.Name = n6;
    ret = D3DXFrameFind(&root, n6);
    ok(ret == &child3, "Unexpected frame, %p.\n", ret);
}

static ID3DXFileData *get_mesh_data(const void *memory, SIZE_T length)
{
    ID3DXFileData *file_data, *ret = NULL;
    ID3DXFileEnumObject *enum_obj = NULL;
    D3DXF_FILELOADMEMORY source;
    ID3DXFile *file;
    SIZE_T i, count;
    GUID guid;

    if (FAILED(D3DXFileCreate(&file)))
        return NULL;

    if (FAILED(file->lpVtbl->RegisterTemplates(file, D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES)))
        goto cleanup;

    source.lpMemory = memory;
    source.dSize = length;
    if (FAILED(file->lpVtbl->CreateEnumObject(file, &source, D3DXF_FILELOAD_FROMMEMORY, &enum_obj)))
        goto cleanup;

    if (FAILED(enum_obj->lpVtbl->GetChildren(enum_obj, &count)))
        goto cleanup;

    for (i = 0; i < count; ++i)
    {
        if (FAILED(enum_obj->lpVtbl->GetChild(enum_obj, i, &file_data)))
            goto cleanup;

        if (SUCCEEDED(file_data->lpVtbl->GetType(file_data, &guid))
                && IsEqualGUID(&guid, &TID_D3DRMMesh))
        {
            ret = file_data;
            break;
        }
        else
        {
            file_data->lpVtbl->Release(file_data);
        }
    }

cleanup:
    if (enum_obj)
        enum_obj->lpVtbl->Release(enum_obj);
    file->lpVtbl->Release(file);

    return ret;
}

static void test_load_skin_mesh_from_xof(void)
{
    static const char simple_xfile[] =
        "xof 0303txt 0032"
        "Mesh {"
            "3;"
            "0.0; 0.0; 0.0;,"
            "0.0; 1.0; 0.0;,"
            "1.0; 1.0; 0.0;;"
            "1;"
            "3; 0, 1, 2;;"
        "}";
    static const char simple_xfile_empty[] =
        "xof 0303txt 0032"
        "Mesh { 0;; 0;; }";
    static const D3DVERTEXELEMENT9 expected_declaration[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END(),
    };
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];
    ID3DXBuffer *adjacency, *materials, *effects;
    DWORD max_influences[3], count, fvf;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9 *device;
    ID3DXSkinInfo *skin_info;
    ID3DXFileData *file_data;
    const char *bone_name;
    D3DXMATRIX *matrix;
    float influence;
    ID3DXMesh *mesh;
    IDirect3D9 *d3d;
    ULONG refcount;
    HRESULT hr;
    HWND hwnd;

    if (!(hwnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Failed to create application window.\n");
        return;
    }

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Failed to create d3d object.\n");
        DestroyWindow(hwnd);
        return;
    }

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    IDirect3D9_Release(d3d);
    if (FAILED(hr))
    {
        skip("Failed to create device, hr %#lx.\n", hr);
        DestroyWindow(hwnd);
        return;
    }

    file_data = get_mesh_data(simple_xfile, sizeof(simple_xfile) - 1);
    ok(!!file_data, "Failed to load mesh data.\n");

    adjacency = materials = effects = (void *)0xdeadbeef;
    count = ~0u;
    skin_info = (void *)0xdeadbeef;
    mesh = (void *)0xdeadbeef;

    hr = D3DXLoadSkinMeshFromXof(file_data, 0, device, &adjacency, &materials, &effects, &count,
            &skin_info, &mesh);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!adjacency, "Got unexpected value %p.\n", adjacency);
    ok(!materials, "Got unexpected value %p.\n", materials);
    ok(!effects, "Got unexpected value %p.\n", effects);
    ok(!count, "Got unexpected value %lu.\n", count);
    ok(!!skin_info, "Got unexpected value %p.\n", skin_info);
    ok(!!mesh, "Got unexpected value %p.\n", mesh);
    count = mesh->lpVtbl->GetNumVertices(mesh);
    ok(count == 3, "Got unexpected value %lu.\n", count);
    count = mesh->lpVtbl->GetNumFaces(mesh);
    ok(count == 1, "Got unexpected value %lu.\n", count);

    hr = skin_info->lpVtbl->GetDeclaration(skin_info, declaration);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    compare_elements(declaration, expected_declaration, __LINE__, 0);

    fvf = skin_info->lpVtbl->GetFVF(skin_info);
    ok(fvf == D3DFVF_XYZ, "Got unexpected value %lu.\n", fvf);

    count = skin_info->lpVtbl->GetNumBones(skin_info);
    ok(!count, "Got unexpected value %lu.\n", count);

    influence = skin_info->lpVtbl->GetMinBoneInfluence(skin_info);
    ok(!influence, "Got unexpected value %.8e.\n", influence);

    memset(max_influences, 0x55, sizeof(max_influences));
    hr = skin_info->lpVtbl->GetMaxVertexInfluences(skin_info, max_influences);
    todo_wine ok(hr == D3D_OK, "Got unexpected value %#lx.\n", hr);
    todo_wine ok(!max_influences[0], "Got unexpected value %lu.\n", max_influences[0]);
    ok(max_influences[1] == 0x55555555, "Got unexpected value %lu.\n", max_influences[1]);
    ok(max_influences[2] == 0x55555555, "Got unexpected value %lu.\n", max_influences[2]);

    bone_name = skin_info->lpVtbl->GetBoneName(skin_info, 0);
    ok(!bone_name, "Got unexpected value %p.\n", bone_name);

    count = skin_info->lpVtbl->GetNumBoneInfluences(skin_info, 0);
    ok(!count, "Got unexpected value %lu.\n", count);

    count = skin_info->lpVtbl->GetNumBoneInfluences(skin_info, 1);
    ok(!count, "Got unexpected value %lu.\n", count);

    matrix = skin_info->lpVtbl->GetBoneOffsetMatrix(skin_info, -1);
    ok(!matrix, "Got unexpected value %p.\n", matrix);

    matrix = skin_info->lpVtbl->GetBoneOffsetMatrix(skin_info, 0);
    ok(!matrix, "Got unexpected value %p.\n", matrix);

    skin_info->lpVtbl->Release(skin_info);
    mesh->lpVtbl->Release(mesh);
    adjacency->lpVtbl->Release(adjacency);
    file_data->lpVtbl->Release(file_data);

    /* Empty Mesh Test */
    file_data = get_mesh_data(simple_xfile_empty, sizeof(simple_xfile_empty) - 1);
    ok(!!file_data, "Failed to load mesh data.\n");

    adjacency = materials = effects = (void *)0xdeadbeef;
    count = 0xdeadbeefu;
    skin_info = (void *)0xdeadbeef;
    mesh = (void *)0xdeadbeef;

    hr = D3DXLoadSkinMeshFromXof(file_data, 0, device, &adjacency, &materials, &effects, &count,
            &skin_info, &mesh);
    todo_wine ok(hr == D3DXERR_LOADEDMESHASNODATA, "Unexpected hr %#lx.\n", hr);
    ok(!adjacency, "Unexpected adjacency %p.\n", adjacency);
    ok(!materials, "Unexpected materials %p.\n", materials);
    ok(!effects, "Unexpected effects %p.\n", effects);
    ok(count == 0xdeadbeefu, "Unexpected count %lu.\n", count);
    ok(skin_info == (void *)0xdeadbeef, "Unexpected skin_info %p.\n", skin_info);
    ok(!mesh, "Unexpected mesh %p.\n", mesh);

    file_data->lpVtbl->Release(file_data);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(hwnd);
}

static void test_mesh_optimize(void)
{
/*
 *   . _ .
 *  / \ / \
 * . _ . _ .
 *  \ / \ /
 *   . _ .
 */
    static const struct
    {
        float c[3];
        float n[3];
        float t[2];
    }
    vertices[] =
    {
        { {-0.5f, -1.0f, 0.0f,}, {0.0f, 0.0f, 1.0f}, {-0.5f, -1.0f} },
        { { 0.5f, -1.0f, 0.0f,}, {0.0f, 0.0f, 1.0f}, { 0.5f, -1.0f} },

        { {-1.0f,  0.0f, 0.0f,}, {0.0f, 0.0f, 1.0f}, {-1.0f,  0.0f} },
        { { 0.0f,  0.0f, 0.0f,}, {0.0f, 0.0f, 1.0f}, { 0.0f,  0.0f} },
        { { 1.0f,  0.0f, 0.0f,}, {0.0f, 0.0f, 1.0f}, { 1.0f,  0.0f} },

        { {-0.5f,  1.0f, 0.0f,}, {0.0f, 0.0f, 1.0f}, {-0.5f,  1.0f} },
        { { 0.5f,  1.0f, 0.0f,}, {0.0f, 0.0f, 1.0f}, { 0.5f,  1.0f} },
    };
    static const unsigned short indices[] =
    {
        3, 0, 2,
        3, 2, 5,
        3, 5, 6,
        3, 6, 4,
        3, 4, 1,
        3, 1, 0,
    };
    static const DWORD attrs[] = { 1, 2, 1, 2, 1, 2 };
    static const DWORD expected_adjacency[] =
    {
        5, 0xffffffff, 1,
        0, 0xffffffff, 2,
        1, 0xffffffff, 3,
        2, 0xffffffff, 4,
        3, 0xffffffff, 5,
        4, 0xffffffff, 0,
    };
    static const DWORD expected_adjacency_out[] =
    {
        5, 0xffffffff, 3,
        3, 0xffffffff, 4,
        4, 0xffffffff, 5,
        0, 0xffffffff, 1,
        1, 0xffffffff, 2,
        2, 0xffffffff, 0,
    };

    DWORD adjacency[6 * 3], adjacency_out[6 * 3];
    struct test_context *test_context;
    IDirect3DDevice9 *device;
    ID3DXBuffer *buffer;
    ID3DXMesh *mesh;
    unsigned int i;
    DWORD size;
    HRESULT hr;
    void *data;

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        return;
    }
    device = test_context->device;

    hr = D3DXCreateMeshFVF(ARRAY_SIZE(attrs), ARRAY_SIZE(vertices), D3DXMESH_VB_MANAGED | D3DXMESH_IB_MANAGED,
            D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, device, &mesh);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, &data);
    ok(hr == S_OK, "got %#lx.\n", hr);
    memcpy(data, vertices, sizeof(vertices));
    hr = mesh->lpVtbl->UnlockVertexBuffer(mesh);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, &data);
    ok(hr == S_OK, "got %#lx.\n", hr);
    memcpy(data, indices, sizeof(indices));
    hr = mesh->lpVtbl->UnlockIndexBuffer(mesh);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = mesh->lpVtbl->LockAttributeBuffer(mesh, 0, (DWORD **)&data);
    ok(hr == S_OK, "got %#lx.\n", hr);
    memcpy(data, attrs, sizeof(attrs));
    hr = mesh->lpVtbl->UnlockAttributeBuffer(mesh);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = mesh->lpVtbl->GenerateAdjacency(mesh, 0.0f, adjacency);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(!memcmp(adjacency, expected_adjacency, sizeof(adjacency)), "data mismatch.\n");

    hr = mesh->lpVtbl->OptimizeInplace(mesh, D3DXMESHOPT_IGNOREVERTS | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_DONOTSPLIT,
            adjacency, adjacency_out, NULL, &buffer);
    ok(hr == S_OK, "got %#lx.\n", hr);

    size = buffer->lpVtbl->GetBufferSize(buffer);
    ok(size == sizeof(DWORD) * ARRAY_SIZE(vertices), "got %lu.\n", size);
    data = buffer->lpVtbl->GetBufferPointer(buffer);
    for (i = 0; i < ARRAY_SIZE(vertices); ++i)
        ok(((DWORD *)data)[i] == i, "i %u, got %lu.\n", i, ((DWORD *)data)[i]);
    ok(!memcmp(adjacency_out, expected_adjacency_out, sizeof(adjacency)), "data mismatch.\n");

    buffer->lpVtbl->Release(buffer);
    mesh->lpVtbl->Release(mesh);
    free_test_context(test_context);
}

START_TEST(mesh)
{
    D3DXBoundProbeTest();
    D3DXComputeBoundingBoxTest();
    D3DXComputeBoundingSphereTest();
    D3DXGetFVFVertexSizeTest();
    D3DXIntersectTriTest();
    D3DXCreateMeshTest();
    D3DXCreateMeshFVFTest();
    D3DXLoadMeshTest();
    D3DXCreateBoxTest();
    D3DXCreatePolygonTest();
    D3DXCreateSphereTest();
    D3DXCreateCylinderTest();
    D3DXCreateTextTest();
    D3DXCreateTorusTest();
    D3DXCreateAnimationControllerTest();
    D3DXCreateKeyframedAnimationSetTest();
    test_get_decl_length();
    test_get_decl_vertex_size();
    test_fvf_decl_conversion();
    D3DXGenerateAdjacencyTest();
    test_update_semantics();
    test_create_skin_info();
    test_convert_adjacency_to_point_reps();
    test_convert_point_reps_to_adjacency();
    test_weld_vertices();
    test_clone_mesh();
    test_valid_mesh();
    test_optimize_faces();
    test_optimize_vertices();
    test_compute_normals();
    test_D3DXFrameFind();
    test_load_skin_mesh_from_xof();
    test_mesh_optimize();
}
