#include "imgproc.h"

#include <vector>
#include <opencv2/core/version.hpp>
#if CV_VERSION_MAJOR >= 3 && CV_VERSION_MINOR > 3
  // Bugfix: Missing channel_type in Mat_ due to deprecated cv::DataType
  #define OPENCV_TRAITS_ENABLE_DEPRECATED
#endif

//#include <opencv2/core/hal/interface.h>  // basic types
//#include <opencv2/core/mat.hpp>
//#include <opencv2/core/types.hpp>  // cv structs
//#include <opencv2/core/cvstd.hpp>  // math, algorythm, ptr...
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>       // template match
#include <opencv2/features2d.hpp>    // feature detection & descriptor
#include <opencv2/calib3d.hpp>       // camera estimation


#ifndef NDEBUG
#include <iostream>
#define MSG_(msg) std::cout << msg << std::endl
#else
#define MSG_(msg)
#endif

inline bool evalLimits(double x, double y, int xLim, int yLim)
{// false == failed Limit test
    return !(cv::min(x, y) < 0 ||
        x > xLim ||
        y > yLim ||
        cv::max(x, y) > std::numeric_limits<int>::max()
        );
}


namespace ImProcU8 {
using namespace cv;

// Maximum number of points used for estimation of location
const int MAX_FOR_PT_ESTIM = 32;

// Feature detector: pixel value difference of 20, filter duplicates, feature window with diameter of 9pxl, circumference 16pxl
// ORB filters out too much features in a sample parent image (pyramid?), we use FAST instead.
Ptr<FastFeatureDetector> gDetPtr = FastFeatureDetector::create(20, true);//, FastFeatureDetector::TYPE_7_12);

// FastFeatureDetector::compute() crashes, we use ORB instead
Ptr<ORB> gDescrPtr = ORB::create();

// Feature matcher: Use hamming distance (NORM_HAMMING==6)
// Crosscheck (Im1ToIm2/Im2ToIm1) crashes with knnMatch().
// Faster FlannBasedMatcher crashes, maybe it does not understand ORB descriptors
Ptr<BFMatcher> gCoplPtr = BFMatcher::create(NORM_HAMMING);
// Note: cv::Ptr is a shared pointer with automatic garbage collection

bool locatePatternIn(const Image& rInSrc, const Image& rInTar, imgArr2I_t aInOutXyLoc, float certaintyPerc)
{
    if (certaintyPerc < 0.02)
    {
        certaintyPerc = 0.02f;
    }
    int w1, w2, h1, h2;
    w1 = rInSrc.aSizes[D_WIDTH];
    h1 = rInSrc.aSizes[D_HEIGHT];
    w2 = rInTar.aSizes[D_WIDTH];
    h2 = rInTar.aSizes[D_HEIGHT];

    int l1, l2, s1, s2;
    l1 = max(w1, h1);
    l2 = max(w2, h2);
    s1 = min(w1, h1);
    s2 = min(w2, h2);

    if (rInSrc.channels != 4 || rInTar.channels != 4)
        return false;  //throw std::invalid_argument;
    // TODO: handle overlapping pattern (can be cut out in several ways)
    if ((w2 > w1) || (h2 > h1))
        return false;
    if ((1 > min(s1, s2)) ||
        (MAX_PATTERN_SIZE < l2)
       )
    {
        return false;  //throw std::invalid_argument;
    }
    // There is no const data ctor for mat, but we use the pointer only for reading
    const Mat csrc(h1, w1, CV_8UC4, const_cast<uchar*>(rInSrc.pDat), rInSrc.lneLenByte);
    const Mat cpat(h2, w2, CV_8UC4, const_cast<uchar*>(rInTar.pDat), rInTar.lneLenByte);
    Mat result(h1 - h2 + 1, w1 - w2 + 1, CV_32FC1);

#ifndef NDEBUG
    int64 ts = getTickCount();
#endif

    /*
        matching methods:
        SQDIFF: pixel difference (substract)
        CCORR:  pixel correlation (product)
        CCOEFF: feature pixel correlation (deviation product)
    */
    if (aInOutXyLoc && ((aInOutXyLoc[COOR_LEFT] & aInOutXyLoc[COOR_TOP]) != 0))
    {// Create a 3x3 subset for less amounts of pixel
        const Mat subs(
            csrc,
            Rect(
                aInOutXyLoc[COOR_LEFT]-(w2>>1)-w2,
                aInOutXyLoc[COOR_TOP]-(h2>>1)-h2,
                (w2<<1)+w2,
                (h2<<1)+h2)
        );
        matchTemplate(subs, cpat, result, TM_CCOEFF_NORMED);
    } else {
        matchTemplate(csrc, cpat, result, TM_CCOEFF_NORMED);
    }
    double minVal, maxVal, range;
    Point exLoc;
    minMaxLoc(result, &minVal, &maxVal, NULL, &exLoc);  // location of extrema

#ifndef NDEBUG
    MSG_("Locate by pattern, time: " << (1000 * (getTickCount() - ts) / getTickFrequency()) << " msec");
#endif

    range = maxVal - minVal;
    // extrema must be high enough and discrete from noise
    if (range > certaintyPerc)
    {
        if (aInOutXyLoc)
        {
            aInOutXyLoc[COOR_LEFT] = exLoc.x + (w2 >> 1);
            aInOutXyLoc[COOR_TOP] = exLoc.y + (h2 >> 1);
        }
        return (abs(maxVal) / range) > certaintyPerc;
    } else
        return false;
}


bool locateFeaturesIn(const Image& rInSrc, const Image& rInTar, imgArr2I_t aInOutXyLoc, bool ratioTest, float maxDistRatio)
{
    int w1, w2, h1, h2;
    w1 = rInSrc.aSizes[D_WIDTH];
    h1 = rInSrc.aSizes[D_HEIGHT];
    w2 = rInTar.aSizes[D_WIDTH];
    h2 = rInTar.aSizes[D_HEIGHT];

    if (rInSrc.channels != 1 || rInTar.channels != 1)
        return false;

    // There is no const data ctor for mat, but we use the pointer only for reading
    const Mat csrc(h1, w1, CV_8U, const_cast<uchar*>(rInSrc.pDat), rInSrc.lneLenByte);
    const Mat cpat(h2, w2, CV_8U, const_cast<uchar*>(rInTar.pDat), rInTar.lneLenByte);

    std::vector<KeyPoint> srcKp, tarKp;
    std::vector<DMatch> matches;
    std::vector<std::vector<DMatch>> matchResults;
    std::vector<Point2f> srcPts, tarPts;
    Mat srcScr, tarScr, transf, src, tar, inlierMap;

#ifndef NDEBUG
    Mat result;
    int64 ts = getTickCount();
    double elapsed = 0;
#endif
    // Resize pattern to find it better within downscaled content.
    resize(cpat, tar, Size(), 0.9, 0.9, INTER_LINEAR_EXACT);
    w2 = tar.cols;
    h2 = tar.rows;

    // Edge detector is vulnerable to image noise
    medianBlur(csrc, src, 3);
    medianBlur(tar, tar, 3);
    //tar = cpat;
    //src = csrc;
    if (aInOutXyLoc && ((aInOutXyLoc[COOR_LEFT] & aInOutXyLoc[COOR_TOP]) != 0))
    {// Create a 3x3 subset for less amounts of pixel
        const Mat subs(
            src,
            Rect(
                aInOutXyLoc[COOR_LEFT] - (w2 >> 1) - w2,
                aInOutXyLoc[COOR_TOP] - (h2 >> 1) - h2,
                (w2 << 1) + w2,
                (h2 << 1) + h2)
        );
        // The max amount of found features is still same
        gDetPtr->detect(subs, srcKp);
        gDescrPtr->compute(subs, srcKp, srcScr);
    } else
    {
        gDetPtr->detect(src, srcKp);
        gDescrPtr->compute(src, srcKp, srcScr);
    }
    gDetPtr->detect(tar, tarKp);
    gDescrPtr->compute(tar, tarKp, tarScr);
    //todo: limit queries to 2k
    if (ratioTest)
    {// For each src descriptor, search the best 2 matches (Crashes with too many queries)
        gCoplPtr->knnMatch(srcScr, tarScr, matchResults, 2);  // tarScr is temporarily trained, not stored in instance
        for (auto it = matchResults.cbegin(); it != matchResults.cend(); it++)
        {
            switch (it->size())
            {
            case 2:  // Skip ambiguous results
                if ((it->at(0).distance / it->at(1).distance) > maxDistRatio)
                {
                    break;
                }
                //[fallthrough]
            case 1:
                // idx0 has always the smallest distance
                matches.emplace_back((*it)[0]);
                break;
            default:  // 0 or more than 2 results
                break;
            }
        }// for all matchResults
    } else {
     // For each src descriptor, search the best match
        gCoplPtr->match(srcScr, tarScr, matches);
    }

    if (matches.size() < 4)
    {// Not enough matches
        return false;
    } else if (matches.size() > MAX_FOR_PT_ESTIM) {
        // Sorting by ascending hamming distance to select the best among
        std::sort(matches.begin(), matches.end(), [&](DMatch a, DMatch b) {return a.distance < b.distance; });
    }
    for (size_t i = 0, upper = min<size_t>(MAX_FOR_PT_ESTIM, matches.size()); i < upper; i++)
    {// Collect coordinates of image features
        srcPts.emplace_back(srcKp[matches[i].queryIdx].pt);
        tarPts.emplace_back(tarKp[matches[i].trainIdx].pt);
    }

    // Calculate 2x3 transformation of pattern to parent coordinate
    // The supplied feature points are filtered using RANSAC==8 method (Removing outliners, error allowance of 1-3.0).
    transf = estimateAffine2D(tarPts, srcPts, inlierMap, RANSAC, 1.5);  // scaled, but no perspective transform

#ifndef NDEBUG
    elapsed = (getTickCount() - ts) / getTickFrequency();
    MSG_("Locate by features, time: " << elapsed*1000 << " msec");
    //drawKeypoints(csrc, srcKp, result);
    drawMatches(
        src, srcKp,
        tar, tarKp,
        std::vector<DMatch>(  // First 10, may not be sorted!
            matches.cbegin(), matches.cbegin()+min<size_t>(10,matches.size())
        ),
        result
    );
#endif

    Scalar_<double> xyLoc(Scalar::all(0));
    if (!transf.empty())
    {
        // Test Transformation
        assert((transf.cols == 3) && (transf.type() == CV_64F));
        xyLoc[0] = transf.at<double>(0, 0);
        xyLoc[1] = transf.at<double>(1, 1);
    }
    if (transf.empty() ||
           (min(xyLoc[0], xyLoc[1]) / max(xyLoc[0], xyLoc[1]) < 0.65)
       )
    {// Transformation not available
        assert((inlierMap.cols == 1) && (inlierMap.type() == CV_8U));
        // Build the average of inliners
        tarPts.clear();
        uchar* pEle = inlierMap.data;
        for (size_t i = 0; i < inlierMap.total(); i++, pEle++)
        {
            if (*pEle)
            {
                tarPts.emplace_back(srcPts[i]);
            }
        }
        if (tarPts.size() < 3)
        {// Result not certain enough
            return false;
        }
        auto xX = std::minmax_element(
            tarPts.cbegin(), tarPts.cend(),
            [&](Point2f a, Point2f b) {return a.x < b.x; }
        );
        auto yY = std::minmax_element(
            tarPts.cbegin(), tarPts.cend(),
            [&](Point2f a, Point2f b) {return a.y < b.y; }
        );
        if (((xX.second->x - xX.first->x) > w2) ||
               ((yY.second->y - yY.first->y) > h2)
           )
        {// Points too far apart
            return false;
        }
        xyLoc = mean(tarPts);
        xyLoc[0] = round(xyLoc[0]);
        xyLoc[1] = round(xyLoc[1]);
    } else {
     // Transformation seems valid
        Mat_<double> loc = transf * (Mat_<double>(3, 1) <<
            (w2 >> 1),
            (h2 >> 1),
            1
        );
        xyLoc[0] = round(loc(0));
        xyLoc[1] = round(loc(1));
    }

    if (!evalLimits(xyLoc[0], xyLoc[1], w1, h1))
    {// Exceed parent borders
        return false;
    }

    if (aInOutXyLoc)
    {
        aInOutXyLoc[COOR_LEFT] = static_cast<int>(xyLoc[0]);
        aInOutXyLoc[COOR_TOP] = static_cast<int>(xyLoc[1]);
    }
    return true;
}

} // namespace ImProcU8