#pragma once


namespace ImProcU8 {

enum { D_WIDTH = 0, D_HEIGHT, D_2AXES};
enum { COOR_LEFT = 0, COOR_TOP, COOR_2DIM };

using imgArr2I_t = int[D_2AXES];    // plain fixed 2dim array of shorts (16B)
using imgPxl_t = unsigned char;     // dept of one color component in a pixel

const int MAX_PATTERN_SIZE = 2048;  // A sane size for lookup pattern (also limits parallelization)
const float IMG_MATCH_SZ_TOL = 0.1f;// Allows 10% image resultion difference for matching alg.

struct Image {
    const imgPxl_t* pDat;    // const data, variable pointer!
    int lneLenByte;          // bytes per row (aligment)
    unsigned char channels;  // Number of color components in one pixel
    imgArr2I_t& aSizes;      // Alternative: int (&aSizes)[2];
    bool volatileData;
};

/**
 Find location of a pixel pattern within another image.
 Pattern size must be smaller than parent image. Both images shall be 4x8bit/pxl.
 If a rough location is given with aInOutXyLoc != {0,0}
 the parent image will be reduced to a smaller search window.
 @param rInSrc         pointer to first pixel (topleft) in the parent image.
 @param rInTar         pointer to first pixel (topleft) in the pattern.
 @param aInOutXyLoc    int[2] estimate location of the pattern within parent image. Can be NULL.
 @param certaintyPerc  percentage of how certain the result should be. Affects scaling tolerance. (0.2-1.0, default .55).
 @return               Result whether the pattern was found.
 */
bool locatePatternIn(const Image& rInSrc, const Image& rInTar, imgArr2I_t aInOutXyLoc=nullptr, float certaintyPerc=0.55f);

bool locateFeaturesIn(const Image& rInSrc, const Image& rInTar, imgArr2I_t aInOutXyLoc = nullptr, bool ratioTest = true, float maxDistRatio = 0.75f);

//void findCropRectIn(const imgPxl_t* pSrcData, imgSize_t& rInOutSrcSz, imgPoint_t& rInOutRectPos);

} // namespace ImProc