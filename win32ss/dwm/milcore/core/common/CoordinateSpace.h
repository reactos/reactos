// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Coordinate Space enums and type collection definitions.
//
//  Notes:
//
//      Coordinate spaces are frequently used to address two-dimensional
//      surfaces represented as two-dimensional arrays of some elements.  The
//      surface might be either raster target (where elements are pixels), such
//      as a display back buffer has, or an array of color values in memory
//      (texture, where elements are referred to as "texels").  We have terms
//      to differentiate the two classes of coordinates spaces used to address
//      these surfaces.  The coordinate space class for addressing raster
//      targets is called Rendering space and the class for addressing a source
//      is called Sampling.  The Sampling spaces include texture addressing but
//      also other spaces for addressing sources that are not texture based.
//
//      The natural unit of such spaces is the distance between neighboring
//      elements.  So if some pixel has coordinates (X,Y), then next-to-right
//      pixel is at (X+1, Y) and next-to-down one is at (X, Y+1), assuming
//      positive X and Y are right and down, respectively.
//
//      Since two-dimensional arrays are commonly used, the first element is
//      usually addressed by (0,0) and establishes the origin of the coordinate
//      space.  Furthermore as these elements are discrete, but can represent
//      physical area in the real world (e.g. LCD pixel) they are given size in
//      the coordinate space.  That size is approximated as a unit in each
//      dimension.  With a continuous coordinate space more points than the
//      singular (0,0) will address the first element.  Therefore a convention
//      should be chosen to specify which coordinates do address a single
//      element.  Two common conventions of addressing fix (0,0) at different
//      locations in the area that addresses the first element.
//
//      The first and most commonly used in this code base fixes (0,0) at the
//      extreme corner of the first element and is the smallest valued
//      coordinate to address it.  The whole set of coordinates that address
//      the first element is (0,0) to (1-,1-), where 1- represents the largest
//      value less than 1.  This range excludes coordinates with 1 in either
//      dimension and is said to be bottom-right exclusive (assuming positive
//      dimensions are right and down.)  The M,N-th element is addressable by
//      (M,N) to (M+1-,N+1-), where M and N are integers.  This leaves half
//      valued coordinates at the center of an element as in (0.5,0.5) and
//      (4.5,9.5) and from there the addressing convention is given its name:
//      Half-Pixel-Center abbreviated HPC.
//
//      Another convention is named Integer-Pixel-Center or IPC.  As the name
//      suggests integer values are found at the center of an elements
//      addressable range.  Following the same bottom-right exclusive
//      convention the coordinate range addressing element M,N is (M-0.5,N-0.5)
//      to (M+0.5-,N+0.5-).
//
//      Though these conventions have names have Half, Integer, and Center in
//      the name the convention can be assumed in more general cases.  For
//      example a coordinate space limited in range from (0,0) to (1,1) and
//      addressing a U by V element texture with texels sizes 1/U by 1/V is
//      said to use Half-Pixel-Center convention.  So a more accurate
//      description for Half-Pixel-Center convention is that integer values
//      only occur between pixel/sample points.
//
//      There are a number of advantages to using Half-Pixel-Center convention,
//      but they will not be called out here.  Integer-Pixel-Center has one
//      particular convenience which is why it used in some parts of this code
//      base.  With IPC convention used for Device Render (destination) and
//      Texel Sampling (source) coordinate spaces and a linear transform
//      between the two, a discrete walk of destination elements (X,Y) ->
//      (X+1,Y) -> (X+2,Y) ... will produce sample coordinates (U,V) that lend
//      themselves readily to address the texels needed for linear
//      interpolation.  Assuming 2D array, Source[ floor(V) ][ floor(U) ] is
//      the base element to sample, with the other neighbors that potentially
//      contribute being +1 in U/V.  And the weight of each sample is directly
//      computed from frac(U) and frac(V).  ( (1-frac(U))*(1-frac(V)) is the
//      weight for the base sample.)
//
//      Another use of Integer-Pixel-Center of note is Direct3D's Render
//      coordinates convention.  However this code base endeavors to convert
//      away from that whenever possible.
//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Enum:
//      CoordinateSpaceId
//
//  Synopsis:
//      Enumeration of coordinate spaces
//
//  Notes:
//      The order of listing is loosely ordered by relativity and tries to
//      follow a common progression:
//          Lowest level Rendering spaces to high level Rendering spaces and
//          then high level Sampling spaces to low level Sampling spaces.
//      Those are followed by intermediate/convenient spaces and IPC outliers.
//
//-----------------------------------------------------------------------------

namespace CoordinateSpaceId
{
    enum Enum
    {
        //
        // Since the HPC and IPC conventions need distinguished and HPC
        // convention is used widely across the code base, integer pixel center
        // associated coordinate spaces are specially marked with "IPC" suffix
        // and the following flag.  Lack of flag indicates a half pixel center
        // associated space or possibly no association whatsoever.
        IntegerPixelCenterFlag = 0x10000000,

        // For use only with uninitialized state and special Variant coord space
        Invalid = 0,


        //+====================================================================
        //
        // D3D Specific Coordinate Spaces
        //

        //+--------------------------------------------------------------------
        //
        // D3D's Homogeneous Clipping (View) coordinate space.
        //
        //  Origin: Center of unclipped (viewport) rectangle.
        //  Unit: 1/2 of unclipped (viewport) target pixels.
        //  Axis directions: Right is positive X; Up is positive Y.
        //  Notable limits: (-1,1) is center of top-leftmost unclipped pixel.
        //                  (1,-1) is center of clipped pixel one pixel down
        //                  and left from bottom-rightmost unclipped pixel.

        //             +1.0
        //       . . . . .+Y . .
        //       .      ^      .
        //       .      |      .
        //       .      |      .
        //       <------O------> +X  +1.0
        //       .      |      .
        //       .      |      .
        //       .      |      .
        //       . . . .v. . . .

        // Homeogenous Clip is considered to have integer pixel center
        // convention because of edge limits.  See
        // CD3DDeviceLevel1::SetSurfaceToClippingMatrix for more details.
        //
        D3DHomogeneousClipIPC = 1 | IntegerPixelCenterFlag,


        //+====================================================================
        //
        // Rasterization / Geometry Coordinate Spaces
        //
        //  The spaces are all half-pixel-center based coordinate systems.
        //  (Integer values only occur between pixel/sample points.)
        //

        //+--------------------------------------------------------------------
        //
        // Device coordinate space.
        //
        //  Origin: Upper-left corner of pixel target.
        //  Unit: Always real physical pixel; Pixel size is 1x1.
        //  Axis directions: Right is positive X; Down is positive Y.
        //  Notable limits: (Width,Height) is bottom-right corner of
        //                  bottom-rightmost target pixel.

        //     O------------> +X  +Width
        //     |            .
        //     |            .
        //     |            .
        //     |            .
        //     |            .
        //     |            .
        //     v +Y . . . . .
        //   +Height

        // The name Device comes from interface these coordinates are sent to.
        // With D3D IDirect3DDevice* is the workhorse interface.  With GDI DCs
        // (Device Contexts) are the primary targets and sources.  The term
        // Device is also used to refer to the graphics hardware in a computer,
        // for which IDirect3DDevice* and DCs are abstractions.
        //
        // Note:
        //      Modern devices and displays can have built-in logic to allow
        //      physical rotation the screen by multiples of 90 degrees.  This
        //      rotation (also called orientation) may or may not be exposed
        //      through to the device abstractions.  By default orientation is
        //      handled by the system allowing caller to works with axes that
        //      continue to be as described above.  However D3D exposes a no
        //      auto-rotate option for fullscreen targets (devices).  When
        //      employed callers must account for rotation themselves.
        //
        //      In this no auto-rotate mode saying "upper-left" means the
        //      original upper-left point as if this point was marked with
        //      pencil.  When the screen is rotated 90 degrees clockwise, "X"
        //      is directed down "Y" directed left.  In other words, device
        //      coordinates are hard tied to display screen.
        //
        DeviceHPC = 2,
        Device = DeviceHPC,


        //+--------------------------------------------------------------------
        //
        // Page* coordinate spaces.
        //
        //  Origin: Upper-left corner of primary pixel target.  For scenarios
        //          with multiple targets being addressed through one
        //          coordinate space and the targets do not have upper-left
        //          corners aligned, then one target is chosen as primary and
        //          origin is fixed to its upper-left corner.
        //  Axis directions: Right is positive X; Down is positive Y.
        //  Notable limits: As secondary targets may correspond to positions
        //                  beyond the primary's bounds, including negative
        //                  positioning relative to primary, the limits are
        //                  defined by the union of all target bounds.
        //                  (Left,Top) is top-left corner of top-leftmost
        //                  "pixel" in union.  (Right,Bottom) is bottom-right
        //                  corner of bottom-rightmost "pixel" in union.

        //          Top
        //       . . . . . . . . . .
        //       .   ^             .
        //       .   |             .
        //  Left <---O-------------> +X  Right
        //       .   |           | .
        //       .   |  Primary  | .
        //       .   |  Target   | .
        //       .   |           | .
        //       .   |-----------+ .
        //       .   |             .
        //       . . v +Y. . . . . .
        //        Bottom

        // These coordinate spaces represents a uniform rendering target above
        // or at the meta level, even when units are pixels.
        //
        // The name Page comes from GDI and GDI+ terminology (see SDK) for
        // space accepting variable units and acting as a uniform space above
        // multiple targets.  GDI uses Page-Space to Device-Space transform to
        // resolve units and convert window coordinates to device coordinates.
        // The two steps are further broken out here.  PageInUnits to
        // PageInPixels resolves units and PageInPixels to Device resolves
        // sub-target conversions.  In the future, PageInPixels to Device
        // transform can be used to resolve DPI (dots per inch) discrepancies
        // between render target groups; since PageInPixels is only meaningful
        // to a single DPI.

        //
        // PageInPixels coordinate space.
        //
        //  Unit: Always pixel like Device, but may not be actual pixel.  They
        //        may be an approximation of pixels that is sufficient to use
        //        for bounding related computations, such as dirty regions.

        //
        // The render target callers, whether they be composition
        // CDrawingContext or an implementation of IMILRenderContext, are
        // responsible for making sure PageInPixels are given to the render
        // target interfaces.  The meta render target is responsible for
        // converting page coordinates to device coordinates whether that be
        // via a transform or direct coordinate manipulation.  The one
        // exception to this rule is when CDrawingContext is communicating
        // directly with a Bitmap Render Target (shortcuts the meta layer).  In
        // such a case CDrawingContext is responsible for supplying state that
        // transforms all the way to Device coordinates.
        //
        PageInPixels,

        //
        // PageInUnits coordinate space.
        //
        //  Unit: Variable, but can be logical pixels, meaning user is DPI
        //        aware.  Note this "logical" pixel is different logical /
        //        device independent pixels describes in Windows Presentation
        //        Foundation SDK.  Logical here means close to a physical pixel
        //        as is the case with PageInPixels.
        //
        // IMILRenderContext implementations that expose Page space with
        // variable units are required to convert to PageInPixels before
        // calling the render target layer.
        //
        PageInUnits,

        //+--------------------------------------------------------------------
        //
        // RootRendering coordinate space.
        //
        //  Origin,Unit,Axes,Limits: Caller defined
        //
        // RootRendering is global base coordinate system relative to page.  It
        // is a space of convenience to allow a caller's view of a page to be
        // modified to fit their needs.  Usually it is relative to PageInUnits.
        //
        // Composition uses RootRendering to PageInPixels transform to convert
        // WVG-units (1/96th inch) to pixels.  Thus it establishes these
        // coordinate system properties:
        //
        //  Origin: Same as PageInPixels - Upper-left corner of primary pixel
        //          target.
        //  Unit: WVG (1/96th inch).  This is described as a logical pixel in
        //        Windows Presentation Foundation documentation.  Note this is
        //        a different logical pixel than the "logical" PageInPixels
        //        pixel.
        //  Axis directions: Right is positive X; Down is positive Y.
        //  Notable limits: Same as PageInPixels but scaled to WVG units.
        //
        RootRendering,

        //+--------------------------------------------------------------------
        //
        // LocalRendering coordinate space.
        //
        //  Origin,Unit,Axes,Limits: Caller defined
        //
        // LocalRendering is coordinate space for simple geometry objects such
        // as shapes and XYZ of mesh vertices.  LocalRendering should be
        // treated as a subset of the global coordinate space.  That is
        // LocalRendering space is a RootRendering space.  RootRendering space
        // is not, however, always a LocalRendering space.  They are separate
        // with this enumeration to highlight that some coordinate work is
        // always performed/tracked with local coordinates.
        //
        // The coordinate values that come to drawing context are assumed
        // to be in local coordinate space.  Example:
        //     DrawingContext.DrawLine(pen, new Point(10,20), ...);
        //   10 and 20 are X- and Y-coordinates in local coordinate space.
        //
        // Caller can place local coordinate system arbitrary way.  The
        // correspondence between local and root coordinates is defined by the
        // chain of transformations along the path from the visual where
        // drawing context is tied, to the root visual of the scene.
        //
        LocalRenderingHPC,
        LocalRendering = LocalRenderingHPC,
        // Local Rendering aliases
        Shape = LocalRendering,
        MeshGeometry = LocalRendering,


        //+====================================================================
        //
        // 3D / Geometry Coordinate Spaces
        //

        //+--------------------------------------------------------------------
        //
        // Viewport3D coordinate space.
        //
        // Viewport3D is root of rendering space for 3D content.  It is a
        // Local"2D"Rendering space and must be related to Page just like
        // LocalRendering.
        //
        Viewport3D = LocalRendering,

        //+--------------------------------------------------------------------
        //
        // HomogeneousClipping / Projection3D coordinate space.
        //
        // HomogeneousClipping is nearly identical to D3D3DHomogeneousClipping. The
        // difference is that HomogeneousClipping uses HPC convention.
        //
        //  Origin: Center of unclipped (viewport) rectangle.
        //  Unit: 1/2 of unclipped (viewport) target pixels.
        //  Axis directions: Right is positive X; Up is positive Y.
        //  Notable limits: (-1,1) is upper-left corner of top-leftmost
        //                  unclipped pixel.  (1,-1) is bottom-right corner of
        //                  bottom-rightmost unclipped pixel.

        //             +1.0
        //       . . . . .+Y . .
        //       .      ^      .
        //       .      |      .
        //       .      |      .
        //       <------O------> +X  +1.0
        //       .      |      .
        //       .      |      .
        //       .      |      .
        //       . . . .v. . . .

        // Projection3D is a particular use of HomogeneousClipping for 3D.
        // Projection3D is the 3D Rendering results projected into a 2D space.
        // That 2D space is arbitrarily chosen to be homeogenous and match
        // established D3D convention.
        //
        // Projection3D to Viewport3D transform relates projected 3D geometry
        // into 2D plane for rasterization.
        //
        HomogeneousClipping,
        Projection3D = HomogeneousClipping,

        //+--------------------------------------------------------------------
        //
        // View3D coordinate space.
        //
        View3D,

        //+--------------------------------------------------------------------
        //
        // World3D coordinate space.
        //
        World3D,

        //+--------------------------------------------------------------------
        //
        // Local3D coordinate space.
        //
        Local3D,


        //+====================================================================
        //
        // Sampling / Source Coordinate Spaces
        //

        //+--------------------------------------------------------------------
        //
        // IdealSampling
        //
        // IdealSampling is coordinate system that is Device space or as close
        // to Device space as can be estimated.
        //
        //  Unit: Ideally one unit of Device space (or one physical pixel). 
        //        Half units ideally map to pixel centers.
        //  Axis directions: Right is positive X; Down is positive Y.
        //
        // The concept of ideal sampling space is used to compute the highest
        // quality source realizations.
        //
        IdealSampling,


        //+--------------------------------------------------------------------
        //
        // BaseSampling coordinate space.
        //
        //  Origin,Unit,Axes,Limits: Caller defined
        //
        // BaseSampling is global base coordinate system for source sampling. 
        // In 2D rendering when sampling coordinates are not supplied
        // LocalRendering space is used as BaseSampling space.  DrawMesh2D/3D
        // are the only primitive that currently provide their own sampling
        // coordinates.
        //
        BaseSampling,
        BaseSamplingHPC = BaseSampling,
        // Base Sampling aliases
        MeshTextureSampling = BaseSampling,
        Effect = BaseSampling,

        //+--------------------------------------------------------------------
        //
        // Unused sampling spaces falling between World and Realization

        //LocalSampling,
        //ViewportSampling =? LocalSampling,
        //ViewboxSampling,

        //+--------------------------------------------------------------------
        //
        // BrushSampling
        //
        // Today there is no difference between BaseSampling and a notion of
        // brush space, but there really should be.  This would enable easier
        // modification of how a brush fills a mesh, for example.
        //
        //BrushSampling,

        //+--------------------------------------------------------------------
        //
        // RealizationSampling coordinate space.
        //
        //  Origin: "Working" upper-left of likely texture
        //  Unit: "Working" texel
        //
        // Discrete based coordinate space for sources that are texel based. 
        // This is nearly identical to TexelSampling though it allows for
        // dynamic modifications due to operations, like prefiltering, that
        // change the coordinate space properties, like Origin and Unit.
        //
        RealizationSampling,

        //+--------------------------------------------------------------------
        //
        // TexelSampling coordinate space.
        //
        // Sample points even if stored in a texture (which makes them texels)
        // should not always be treated as rectangles.  That value of the
        // sample points may have been set using a wide variety of methods and
        // that storage method directly impacts how the original image can be
        // most accurately reconstructed.
        //
        // Yet thinking of sample points as a rectangle can be a very useful
        // analogy and in very common cases produces the same results as if
        // samples were treated as points.  Nearest neighbor (Point) and Linear
        // filtering are compatible with samples are rectangles.
        //
        // For convenience references to "Texel Rect" mean texels treated as
        // rectangles.
        //
        //  Origin: Upper-left corner of upper-left texel rect (sample point if
        //          they were treated as rectangles).
        //  Unit: "Bounds" of a discrete texel rect; the size of texel rect.
        //  Axis directions: Right is positive U; Down is positive V.
        //  Notable limits: (Width,Height) is bottom-right corner of
        //                  bottom-rightmost texel rect.

        //     O--------------> +U  +Width
        //     |      `       .
        //     |   x  `   x   .
        //     |      `       .
        //     |- - - - - - - .
        //     |      `       .
        //     |   x  `   x   .
        //     |      `       .
        //     v +V  .` . . . .
        //   +Height

        // Scaling by 1/Width and 1/Height produces TextureSampling
        // coordinates.
        //
        TexelSampling,
        TexelSamplingHPC = TexelSampling,

        //+--------------------------------------------------------------------
        //
        // TextureSampling
        //
        //  Origin: Upper-left corner of upper-left texel rect (sample point if
        //          they were treated as rectangles).
        //  Unit: "Bounds" of texture with texels as rectangles.
        //  Axis directions: Right is positive u; Down is positive v.
        //  Notable limits: (1,1) is bottom-right corner of
        //                  bottom-rightmost texel rect.

        //     O--------------> +u  +1
        //     |      `       .
        //     |   x  `   x   .
        //     |      `       .
        //     |- - - - - - - .
        //     |      `       .
        //     |   x  `   x   .
        //     |      `       .
        //     v +v  .` . . . .
        //    +1

        // Normalized sampling space where the unit square matches the bounds
        // of the natural texture.  It differs from TexelSampling only by
        // scaling factors.
        TextureSampling,


        //+====================================================================
        //
        // Intermediate coordinate spaces of convenience
        //

        //+--------------------------------------------------------------------
        //
        // Inches
        //
        //  Unit: inch
        //
        Inches,


        //+====================================================================
        //
        // Integer Pixel Center Coordinate Spaces
        //

        //+--------------------------------------------------------------------
        //
        // DeviceIPC coordinate space.
        //
        // Just like DeviceHPC, but using integer pixel center convention.
        //
        //  Origin: Center of upper-left pixel of pixel target.
        //  Unit: Always real physical pixel; Pixel size is 1x1.
        //  Axis directions: Right is positive X; Down is positive Y.
        //  Notable limits: (-1/2,-1/2) is top-left corner of top-leftmost
        //                  target pixel.  (Width-1/2,Height-1/2) is
        //                  bottom-right corner of bottom-rightmost target
        //                  pixel.

        //     -1/2
        // -1/2 . . . . . . . .
        //      .O------------> +X  +Width-1/2
        //      .|            .
        //      .|            .
        //      .|            .
        //      .|            .
        //      .|            .
        //      .|            .
        //      .v +Y . . . . .
        //     +Height-1/2
        //
        DeviceIPC = DeviceHPC | IntegerPixelCenterFlag,
    };
}

//+----------------------------------------------------------------------------
//
//  Type Collection:
//      CoordinateSpace
//
//  Synopsis:
//      Unique types for Coordinate Spaces
//
//-----------------------------------------------------------------------------

namespace CoordinateSpace
{
    #define DEFINE_SPACE(name) \
        struct name { static const CoordinateSpaceId::Enum Id = CoordinateSpaceId::name; }

    DEFINE_SPACE(D3DHomogeneousClipIPC);

    DEFINE_SPACE(DeviceHPC);
    typedef DeviceHPC Device;

    DEFINE_SPACE(PageInPixels);
    DEFINE_SPACE(PageInUnits);
    DEFINE_SPACE(RootRendering);
    DEFINE_SPACE(LocalRenderingHPC);
    typedef LocalRenderingHPC LocalRendering;
    typedef LocalRendering Shape;

    DEFINE_SPACE(Viewport3D);
    DEFINE_SPACE(HomogeneousClipping);
    typedef HomogeneousClipping Projection3D;

    DEFINE_SPACE(View3D);
    DEFINE_SPACE(World3D);
    DEFINE_SPACE(Local3D);

    DEFINE_SPACE(IdealSampling);

    DEFINE_SPACE(BaseSampling);
    typedef BaseSampling BaseSamplingHPC;
    typedef BaseSampling Effect;

    //DEFINE_SPACE(LocalSampling);
    //DEFINE_SPACE(ViewportSampling);
    //DEFINE_SPACE(ViewboxSampling);

    //DEFINE_SPACE(BrushSampling);
    DEFINE_SPACE(RealizationSampling);
    DEFINE_SPACE(TexelSampling);
    typedef TexelSampling TexelSamplingHPC;
    DEFINE_SPACE(TextureSampling);

    DEFINE_SPACE(Inches);

    DEFINE_SPACE(DeviceIPC);

    //
    // Variant is a special coordinate space type.  Its use should be limited.
    // Variant coordinate space is used to store a type that is one of any
    // other valid coordinate space.  Some example uses:
    //
    //   CAdjustTransform modifies OutCoordSpace for a transforms used with
    //   multiple RTs.  The InCoordSpace of the transform it adjusts does not
    //   matter.  And since it needs to handle multiple InCoordSpaces depending
    //   on run time context we store the transforms with Variant InCoordSpace.
    //
    //   CDelayComputedBounds produces bounds in a specific result coordinate
    //   space, but the coordinate space of starting bounds and transforms to
    //   convert bounds don't matter as long as given bounds match In/Out of
    //   transform and Out/In of transform matches result, respectively.  Since
    //   multiple starting bound type can be given Variant is used.
    //

    struct Variant { static const CoordinateSpaceId::Enum Id = CoordinateSpaceId::Invalid; };
}


//+----------------------------------------------------------------------------
//
//  Diagrams of most common coordinate transforms
//
//-----------------------------------------------------------------------------
//  2D Rasterization - Local shape to Target pixels
//-----------------------------------------------------------------------------
//
//   LocalRendering -> RootRendering -> Page -> Device
//
//
//  When used by Composition with limited DPI support
// ----------------------------------------------------
//
//   LocalRendering -> RootRendering -----> PageInPixels ------> Device
//                                    DPI                 Meta
//
//
//  When used by unmanaged dev tests
// ----------------------------------
//
//   LocalRendering-> RootRendering-> PageInUnits---> PageInPixels----> Device
//                                               DPI              Meta
//
//-----------------------------------------------------------------------------
//  Texture Based Source Sampling
//-----------------------------------------------------------------------------
//
//   BaseSampling -> RealizationSampling -> TexelSampling -> TextureSampling
//
//-----------------------------------------------------------------------------
//  2D Source Sampling with direct Rendering to Sampling mapping
//-----------------------------------------------------------------------------
//
//   Device -> -> {LocalRendering == BaseSampling} -> -> TextureSampling
//
//-----------------------------------------------------------------------------


