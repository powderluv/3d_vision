#ifndef TWO_FRAME_POSE_H
#define TWO_FRAME_POSE_H

#include<opencv2/opencv.hpp>

namespace orb_slam
{
typedef std::pair<int,int> Match;

void FindRT(std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2, std::vector<bool> &vbMatchesInliers,
            cv::Mat &R21, cv::Mat &t21, std::vector<Match>& mvMatches12, cv::Mat mK, cv::Mat debug_img
);

void FindHomography(std::vector<bool> &vbMatchesInliers, float &score, cv::Mat &H21,
                     std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                     std::vector<std::vector<size_t>>& mvSets, int mMaxIterations, float mSigma,
                     std::vector<std::pair<int,int>>& mvMatches12
);
void FindFundamental(std::vector<bool> &vbInliers, float &score, cv::Mat &F21,
                     std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                     std::vector<std::vector<size_t>>& mvSets, int mMaxIterations, float mSigma,
                     std::vector<std::pair<int,int>>& mvMatches12
);

cv::Mat ComputeH21(const std::vector<cv::Point2f> &vP1, const std::vector<cv::Point2f> &vP2);
cv::Mat ComputeF21(const std::vector<cv::Point2f> &vP1, const std::vector<cv::Point2f> &vP2);

float CheckHomography(const cv::Mat &H21, const cv::Mat &H12, std::vector<bool> &vbMatchesInliers, float sigma,
                      std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                      std::vector<Match>& mvMatches12
);

float CheckFundamental(const cv::Mat &F21, std::vector<bool> &vbMatchesInliers, float sigma,
                       std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                       std::vector<Match>& mvMatches12, bool debug
);

bool ReconstructF(std::vector<bool> &vbMatchesInliers, cv::Mat &F21, cv::Mat &K,
                    cv::Mat &R21, cv::Mat &t21, std::vector<cv::Point3f> &vP3D, std::vector<bool> &vbTriangulated, 
                  std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2, float mSigma,
                  float minParallax, int minTriangulated, std::vector<Match>& mvMatches12);

bool ReconstructH(std::vector<bool> &vbMatchesInliers, cv::Mat &H21, cv::Mat &K,
                    cv::Mat &R21, cv::Mat &t21, std::vector<cv::Point3f> &vP3D, std::vector<bool> &vbTriangulated,
                  std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2, float mSigma,
                  float minParallax, int minTriangulated, std::vector<Match>& mvMatches12);

void Triangulate(const cv::KeyPoint &kp1, const cv::KeyPoint &kp2, const cv::Mat &P1, const cv::Mat &P2, cv::Mat &x3D);

void Normalize(const std::vector<cv::KeyPoint> &vKeys, std::vector<cv::Point2f> &vNormalizedPoints, cv::Mat &T);

int CheckRT(const cv::Mat &R, const cv::Mat &t, const std::vector<cv::KeyPoint> &vKeys1, const std::vector<cv::KeyPoint> &vKeys2,
                    const std::vector<Match> &vMatches12, std::vector<bool> &vbInliers,
                    const cv::Mat &K, std::vector<cv::Point3f> &vP3D, float th2, std::vector<bool> &vbGood, float &parallax);

void DecomposeE(const cv::Mat &E, cv::Mat &R1, cv::Mat &R2, cv::Mat &t);

void ExtractOrb(std::string img_name, cv::Mat& desc_list, std::vector<cv::KeyPoint>& kps_list,
                    std::vector<std::vector<std::vector<std::size_t>>>& mGrid,
                    cv::Mat cam_m, cv::Mat cam_dis);

void ExtractOrb(cv::Mat img, cv::Mat& desc_list, std::vector<cv::KeyPoint>& kps_list,
                    std::vector<std::vector<std::vector<std::size_t>>>& mGrid,
                    cv::Mat cam_m, cv::Mat cam_dis);
}


  

#endif