#include "orb_slam_lib/two_frame_pose.h"
#include "orb_slam_lib/ORBextractor.h"
#include "orb_slam_lib/ORBmatcher.h"
#include <Eigen/Dense>
namespace orb_slam
{
    void FindRT(std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2, std::vector<bool> &vbMatchesInliers,
                cv::Mat &R21, cv::Mat &t21, std::vector<Match>& mvMatches12, cv::Mat mK, cv::Mat debug_img
    ){
        //R21=cv::Mat::eye(3, 3, CV_32FC1);
        //t21=cv::Mat::zeros(3, 1, CV_32FC1);
        int mMaxIterations=200;
        float mSigma =1;
        const int N = mvMatches12.size();
        std::vector<bool> vbMatchesInliersH, vbMatchesInliersF;
        vbMatchesInliersF = std::vector<bool>(N ,false);
        std::vector< std::vector<size_t> > mvSets = std::vector< std::vector<size_t> >(mMaxIterations,std::vector<size_t>(8,0));
        
        std::vector<size_t> vAllIndices;
        vAllIndices.reserve(N);
        std::vector<size_t> vAvailableIndices;

        for(int i=0; i<N; i++)
        {
            vAllIndices.push_back(i);
        }
        
        for(int it=0; it<mMaxIterations; it++)
        {
            vAvailableIndices = vAllIndices;

            // Select a minimum set
            for(size_t j=0; j<8; j++)
            {
                int min=0;
                int max=vAvailableIndices.size()-1;
                int randi = min + (rand() % static_cast<int>(max - min + 1));
                int idx = vAvailableIndices[randi];

                mvSets[it][j] = idx;

                vAvailableIndices[randi] = vAvailableIndices.back();
                vAvailableIndices.pop_back();
            }
        }
        float SH=0, SF;
        cv::Mat H, F;
        FindFundamental(vbMatchesInliersF, SF, F, mvKeys1, mvKeys2, mvSets, mMaxIterations, mSigma, mvMatches12);
        FindHomography(vbMatchesInliersH, SH, H, mvKeys1, mvKeys2, mvSets, mMaxIterations, mSigma, mvMatches12);
        
    //     cv::cvtColor(debug_img, debug_img, cv::COLOR_GRAY2BGR);
    //     for(int i=0; i<vbMatchesInliersF.size(); i++){
    //         if(vbMatchesInliersF[i]==true){
    //             int ind1=mvMatches12[i].first;
    //             int ind2=mvMatches12[i].second;
    //             cv::line(debug_img, mvKeys1[ind1].pt, mvKeys2[ind2].pt, CV_RGB(255,0,255));
    //         }
    //     }
    //     for(int i=0; i<mvMatches12.size(); i++){
    //         int ind1=mvMatches12[i].first;
    //         int ind2=mvMatches12[i].second;
    //         cv::line(debug_img, mvKeys1[ind1].pt, mvKeys2[ind2].pt, CV_RGB(255,0,255));
    //     }
        
    //     for(int i=0; i<mvKeys1.size(); i++){
    //         cv::circle(debug_img, mvKeys1[i].pt, 1, CV_RGB(0,0,255), 2);
    //     }
        
    //     cv::imshow("chamo", debug_img);
    //     cv::waitKey(-1);

        //std::cout<<SH<<":"<<SF<<std::endl;

        float RH = SH/(SH+SF);

        // Try to reconstruct from homography or fundamental depending on the ratio (0.40-0.45)
    //     if(RH>0.60){
    //         std::cout<<"initial with Homegrahpy"<<std::endl;
    //     }else{
    //         std::cout<<"initial with Foundamental"<<std::endl;
    //     }
        std::vector<cv::Point3f> vP3D;
        std::vector<bool> vbTriangulated;
        if(RH>0.40){
            ReconstructH(vbMatchesInliersH,H,mK,R21,t21,vP3D,vbTriangulated, mvKeys1, mvKeys2, mSigma, 1.0,50, mvMatches12);
        }else{
            ReconstructF(vbMatchesInliersF,F,mK,R21,t21,vP3D,vbTriangulated, mvKeys1, mvKeys2, mSigma, 1.0,50, mvMatches12);
        }
    }

    void FindHomography(std::vector<bool> &vbMatchesInliers, float &score, cv::Mat &H21,
                        std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                        std::vector<std::vector<size_t>>& mvSets, int mMaxIterations, float mSigma,
                        std::vector<Match>& mvMatches12)
    {
        // Number of putative matches
        const int N = mvMatches12.size();

        // Normalize coordinates
        std::vector<cv::Point2f> vPn1, vPn2;
        cv::Mat T1, T2;
        Normalize(mvKeys1,vPn1, T1);
        Normalize(mvKeys2,vPn2, T2);
        cv::Mat T2inv = T2.inv();

        // Best Results variables
        score = 0.0;
        vbMatchesInliers = std::vector<bool>(N,false);

        // Iteration variables
        std::vector<cv::Point2f> vPn1i(8);
        std::vector<cv::Point2f> vPn2i(8);
        cv::Mat H21i, H12i;
        std::vector<bool> vbCurrentInliers(N,false);
        float currentScore;

        // Perform all RANSAC iterations and save the solution with highest score
        for(int it=0; it<mMaxIterations; it++)
        {
            // Select a minimum set
            for(size_t j=0; j<8; j++)
            {
                int idx = mvSets[it][j];

                vPn1i[j] = vPn1[mvMatches12[idx].first];
                vPn2i[j] = vPn2[mvMatches12[idx].second];
            }

            cv::Mat Hn = ComputeH21(vPn1i,vPn2i);
            H21i = T2inv*Hn*T1;
            H12i = H21i.inv();

            currentScore = CheckHomography(H21i, H12i, vbCurrentInliers, mSigma, mvKeys1, mvKeys2, mvMatches12);

            if(currentScore>score)
            {
                H21 = H21i.clone();
                vbMatchesInliers = vbCurrentInliers;
                score = currentScore;
            }
        }
    }


    void FindFundamental(std::vector<bool> &vbMatchesInliers, float &score, cv::Mat &F21,
                        std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                        std::vector<std::vector<size_t>>& mvSets, int mMaxIterations, float mSigma,
                        std::vector<std::pair<int,int>>& mvMatches12 )
    {
        // Number of putative matches
        const int N = vbMatchesInliers.size();

        // Normalize coordinates
        std::vector<cv::Point2f> vPn1, vPn2;
        cv::Mat T1, T2;
        Normalize(mvKeys1,vPn1, T1);
        Normalize(mvKeys2,vPn2, T2);
        cv::Mat T2t = T2.t();

        // Best Results variables
        score = 0.0;
        vbMatchesInliers = std::vector<bool>(N,false);

        // Iteration variables
        std::vector<cv::Point2f> vPn1i(8);
        std::vector<cv::Point2f> vPn2i(8);
        cv::Mat F21i;
        std::vector<bool> vbCurrentInliers(N,false);
        float currentScore;

        // Perform all RANSAC iterations and save the solution with highest score
        for(int it=0; it<mMaxIterations; it++)
        {
            // Select a minimum set
            for(int j=0; j<8; j++)
            {
                int idx = mvSets[it][j];

                vPn1i[j] = vPn1[mvMatches12[idx].first];
                vPn2i[j] = vPn2[mvMatches12[idx].second];
            }

            cv::Mat Fn = ComputeF21(vPn1i,vPn2i);

            F21i = T2t*Fn*T1;

            currentScore = CheckFundamental(F21i, vbCurrentInliers, mSigma, mvKeys1, mvKeys2,mvMatches12, false);

            if(currentScore>score)
            {
                F21 = F21i.clone();
                vbMatchesInliers = vbCurrentInliers;
                score = currentScore;
                CheckFundamental(F21i, vbCurrentInliers, mSigma, mvKeys1, mvKeys2,mvMatches12, true);
            }
        }
    }


    cv::Mat ComputeH21(const std::vector<cv::Point2f> &vP1, const std::vector<cv::Point2f> &vP2)
    {
        const int N = vP1.size();

        cv::Mat A(2*N,9,CV_32F);

        for(int i=0; i<N; i++)
        {
            const float u1 = vP1[i].x;
            const float v1 = vP1[i].y;
            const float u2 = vP2[i].x;
            const float v2 = vP2[i].y;

            A.at<float>(2*i,0) = 0.0;
            A.at<float>(2*i,1) = 0.0;
            A.at<float>(2*i,2) = 0.0;
            A.at<float>(2*i,3) = -u1;
            A.at<float>(2*i,4) = -v1;
            A.at<float>(2*i,5) = -1;
            A.at<float>(2*i,6) = v2*u1;
            A.at<float>(2*i,7) = v2*v1;
            A.at<float>(2*i,8) = v2;

            A.at<float>(2*i+1,0) = u1;
            A.at<float>(2*i+1,1) = v1;
            A.at<float>(2*i+1,2) = 1;
            A.at<float>(2*i+1,3) = 0.0;
            A.at<float>(2*i+1,4) = 0.0;
            A.at<float>(2*i+1,5) = 0.0;
            A.at<float>(2*i+1,6) = -u2*u1;
            A.at<float>(2*i+1,7) = -u2*v1;
            A.at<float>(2*i+1,8) = -u2;

        }

        cv::Mat u,w,vt;

        cv::SVDecomp(A,w,u,vt,cv::SVD::MODIFY_A | cv::SVD::FULL_UV);

        return vt.row(8).reshape(0, 3);
    }

    cv::Mat ComputeF21(const std::vector<cv::Point2f> &vP1,const std::vector<cv::Point2f> &vP2)
    {
        const int N = vP1.size();

        cv::Mat A(N,9,CV_32F);

        for(int i=0; i<N; i++)
        {
            const float u1 = vP1[i].x;
            const float v1 = vP1[i].y;
            const float u2 = vP2[i].x;
            const float v2 = vP2[i].y;

            A.at<float>(i,0) = u2*u1;
            A.at<float>(i,1) = u2*v1;
            A.at<float>(i,2) = u2;
            A.at<float>(i,3) = v2*u1;
            A.at<float>(i,4) = v2*v1;
            A.at<float>(i,5) = v2;
            A.at<float>(i,6) = u1;
            A.at<float>(i,7) = v1;
            A.at<float>(i,8) = 1;
        }

        cv::Mat u,w,vt;

        cv::SVDecomp(A,w,u,vt,cv::SVD::MODIFY_A | cv::SVD::FULL_UV);

        cv::Mat Fpre = vt.row(8).reshape(0, 3);

        cv::SVDecomp(Fpre,w,u,vt,cv::SVD::MODIFY_A | cv::SVD::FULL_UV);

        w.at<float>(2)=0;

        return  u*cv::Mat::diag(w)*vt;
    }

    float CheckHomography(const cv::Mat &H21, const cv::Mat &H12, std::vector<bool> &vbMatchesInliers, float sigma, 
                        std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                        std::vector<Match>& mvMatches12)
    {   
        const int N = mvMatches12.size();

        const float h11 = H21.at<float>(0,0);
        const float h12 = H21.at<float>(0,1);
        const float h13 = H21.at<float>(0,2);
        const float h21 = H21.at<float>(1,0);
        const float h22 = H21.at<float>(1,1);
        const float h23 = H21.at<float>(1,2);
        const float h31 = H21.at<float>(2,0);
        const float h32 = H21.at<float>(2,1);
        const float h33 = H21.at<float>(2,2);

        const float h11inv = H12.at<float>(0,0);
        const float h12inv = H12.at<float>(0,1);
        const float h13inv = H12.at<float>(0,2);
        const float h21inv = H12.at<float>(1,0);
        const float h22inv = H12.at<float>(1,1);
        const float h23inv = H12.at<float>(1,2);
        const float h31inv = H12.at<float>(2,0);
        const float h32inv = H12.at<float>(2,1);
        const float h33inv = H12.at<float>(2,2);

        vbMatchesInliers.resize(N);

        float score = 0;

        const float th = 5.991;

        const float invSigmaSquare = 1.0/(sigma*sigma);

        for(int i=0; i<N; i++)
        {
            bool bIn = true;

            const cv::KeyPoint &kp1 = mvKeys1[mvMatches12[i].first];
            const cv::KeyPoint &kp2 = mvKeys2[mvMatches12[i].second];

            const float u1 = kp1.pt.x;
            const float v1 = kp1.pt.y;
            const float u2 = kp2.pt.x;
            const float v2 = kp2.pt.y;

            // Reprojection error in first image
            // x2in1 = H12*x2

            const float w2in1inv = 1.0/(h31inv*u2+h32inv*v2+h33inv);
            const float u2in1 = (h11inv*u2+h12inv*v2+h13inv)*w2in1inv;
            const float v2in1 = (h21inv*u2+h22inv*v2+h23inv)*w2in1inv;

            const float squareDist1 = (u1-u2in1)*(u1-u2in1)+(v1-v2in1)*(v1-v2in1);

            const float chiSquare1 = squareDist1*invSigmaSquare;

            if(chiSquare1>th)
                bIn = false;
            else
                score += th - chiSquare1;

            // Reprojection error in second image
            // x1in2 = H21*x1

            const float w1in2inv = 1.0/(h31*u1+h32*v1+h33);
            const float u1in2 = (h11*u1+h12*v1+h13)*w1in2inv;
            const float v1in2 = (h21*u1+h22*v1+h23)*w1in2inv;

            const float squareDist2 = (u2-u1in2)*(u2-u1in2)+(v2-v1in2)*(v2-v1in2);

            const float chiSquare2 = squareDist2*invSigmaSquare;

            if(chiSquare2>th)
                bIn = false;
            else
                score += th - chiSquare2;

            if(bIn)
                vbMatchesInliers[i]=true;
            else
                vbMatchesInliers[i]=false;
        }

        return score;
    }

    float CheckFundamental(const cv::Mat &F21, std::vector<bool> &vbMatchesInliers, float sigma,
                        std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2,
                        std::vector<Match>& mvMatches12, bool debug
    )
    {
        const int N = mvMatches12.size();

        const float f11 = F21.at<float>(0,0);
        const float f12 = F21.at<float>(0,1);
        const float f13 = F21.at<float>(0,2);
        const float f21 = F21.at<float>(1,0);
        const float f22 = F21.at<float>(1,1);
        const float f23 = F21.at<float>(1,2);
        const float f31 = F21.at<float>(2,0);
        const float f32 = F21.at<float>(2,1);
        const float f33 = F21.at<float>(2,2);

        vbMatchesInliers.resize(N);

        float score = 0;

        const float th = 3.841;
        const float thScore = 5.991;

        const float invSigmaSquare = 1.0/(sigma*sigma);
        if(debug){
            //std::cout<<"================"<<std::endl;
        }
        
        int inlier_count=0;
        
        for(int i=0; i<N; i++)
        {
            bool bIn = true;

            const cv::KeyPoint &kp1 = mvKeys1[mvMatches12[i].first];
            const cv::KeyPoint &kp2 = mvKeys2[mvMatches12[i].second];

            const float u1 = kp1.pt.x;
            const float v1 = kp1.pt.y;
            const float u2 = kp2.pt.x;
            const float v2 = kp2.pt.y;

            // Reprojection error in second image
            // l2=F21x1=(a2,b2,c2)

            const float a2 = f11*u1+f12*v1+f13;
            const float b2 = f21*u1+f22*v1+f23;
            const float c2 = f31*u1+f32*v1+f33;

            const float num2 = a2*u2+b2*v2+c2;

            const float squareDist1 = num2*num2/(a2*a2+b2*b2);

            const float chiSquare1 = squareDist1*invSigmaSquare;

            if(debug){
                //std::cout<<"chiSquare1: "<<chiSquare1<<std::endl;
            }
            
            if(chiSquare1>th)
                bIn = false;
            else
                score += thScore - chiSquare1;

            // Reprojection error in second image
            // l1 =x2tF21=(a1,b1,c1)

            const float a1 = f11*u2+f21*v2+f31;
            const float b1 = f12*u2+f22*v2+f32;
            const float c1 = f13*u2+f23*v2+f33;

            const float num1 = a1*u1+b1*v1+c1;

            const float squareDist2 = num1*num1/(a1*a1+b1*b1);

            const float chiSquare2 = squareDist2*invSigmaSquare;

            if(chiSquare2>th)
                bIn = false;
            else
                score += thScore - chiSquare2;

            if(bIn){
                inlier_count++;
                vbMatchesInliers[i]=true;
            }
            else
                vbMatchesInliers[i]=false;
        }
        if(debug){
            //std::cout<<"Inlier: "<<inlier_count<<std::endl;
        }
        return score;
    }

    bool ReconstructF(std::vector<bool> &vbMatchesInliers, cv::Mat &F21, cv::Mat &K,
                    cv::Mat &R21, cv::Mat &t21, std::vector<cv::Point3f> &vP3D, std::vector<bool> &vbTriangulated, 
                    std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2, float mSigma, 
                    float minParallax, int minTriangulated, std::vector<Match>& mvMatches12)
    {
        int N=0;
        for(size_t i=0, iend = vbMatchesInliers.size() ; i<iend; i++)
            if(vbMatchesInliers[i])
                N++;

        // Compute Essential Matrix from Fundamental Matrix
        cv::Mat E21 = K.t()*F21*K;

        cv::Mat R1, R2, t;

        // Recover the 4 motion hypotheses
        DecomposeE(E21,R1,R2,t);  

        cv::Mat t1=t;
        cv::Mat t2=-t;

        // Reconstruct with the 4 hyphoteses and check
        std::vector<cv::Point3f> vP3D1, vP3D2, vP3D3, vP3D4;
        std::vector<bool> vbTriangulated1,vbTriangulated2,vbTriangulated3, vbTriangulated4;
        float parallax1,parallax2, parallax3, parallax4;

        int nGood1 = CheckRT(R1,t1,mvKeys1,mvKeys2,mvMatches12,vbMatchesInliers,K, vP3D1, 4.0*mSigma*mSigma, vbTriangulated1, parallax1);
        int nGood2 = CheckRT(R2,t1,mvKeys1,mvKeys2,mvMatches12,vbMatchesInliers,K, vP3D2, 4.0*mSigma*mSigma, vbTriangulated2, parallax2);
        int nGood3 = CheckRT(R1,t2,mvKeys1,mvKeys2,mvMatches12,vbMatchesInliers,K, vP3D3, 4.0*mSigma*mSigma, vbTriangulated3, parallax3);
        int nGood4 = CheckRT(R2,t2,mvKeys1,mvKeys2,mvMatches12,vbMatchesInliers,K, vP3D4, 4.0*mSigma*mSigma, vbTriangulated4, parallax4);

        int maxGood = std::max(nGood1,std::max(nGood2,std::max(nGood3,nGood4)));
        
        //std::cout<<nGood1<<":"<<nGood2<<":"<<nGood3<<":"<<nGood4<<std::endl;
        //std::cout<<parallax1<<":"<<parallax2<<":"<<parallax3<<":"<<parallax4<<std::endl;

        int nMinGood = std::max(static_cast<int>(0.9*N),minTriangulated);

        int nsimilar = 0;
        if(nGood1>0.7*maxGood)
            nsimilar++;
        if(nGood2>0.7*maxGood)
            nsimilar++;
        if(nGood3>0.7*maxGood)
            nsimilar++;
        if(nGood4>0.7*maxGood)
            nsimilar++;

        // If there is not a clear winner or not enough triangulated points reject initialization
        if(maxGood<nMinGood || nsimilar>1)
        {
            return false;
        }

        // If best reconstruction has enough parallax initialize
        if(maxGood==nGood1)
        {
            if(parallax1>minParallax)
            {
                vP3D = vP3D1;
                vbTriangulated = vbTriangulated1;

                R1.copyTo(R21);
                t1.copyTo(t21);
                return true;
            }
        }else if(maxGood==nGood2)
        {
            if(parallax2>minParallax)
            {
                vP3D = vP3D2;
                vbTriangulated = vbTriangulated2;

                R2.copyTo(R21);
                t1.copyTo(t21);
                return true;
            }
        }else if(maxGood==nGood3)
        {
            if(parallax3>minParallax)
            {
                vP3D = vP3D3;
                vbTriangulated = vbTriangulated3;

                R1.copyTo(R21);
                t2.copyTo(t21);
                return true;
            }
        }else if(maxGood==nGood4)
        {
            if(parallax4>minParallax)
            {
                vP3D = vP3D4;
                vbTriangulated = vbTriangulated4;

                R2.copyTo(R21);
                t2.copyTo(t21);
                return true;
            }
        }
        return false;
    }

    bool ReconstructH(std::vector<bool> &vbMatchesInliers, cv::Mat &H21, cv::Mat &K,
                    cv::Mat &R21, cv::Mat &t21, std::vector<cv::Point3f> &vP3D, std::vector<bool> &vbTriangulated,
                    std::vector<cv::KeyPoint>& mvKeys1, std::vector<cv::KeyPoint>& mvKeys2, float mSigma,
                    float minParallax, int minTriangulated, std::vector<Match>& mvMatches12)
    {
        int N=0;
        for(size_t i=0, iend = vbMatchesInliers.size() ; i<iend; i++)
            if(vbMatchesInliers[i])
                N++;
        // We recover 8 motion hypotheses using the method of Faugeras et al.
        // Motion and structure from motion in a piecewise planar environment.
        // International Journal of Pattern Recognition and Artificial Intelligence, 1988

        cv::Mat invK = K.inv();
        cv::Mat A = invK*H21*K;

        cv::Mat U,w,Vt,V;
        cv::SVD::compute(A,w,U,Vt,cv::SVD::FULL_UV);
        V=Vt.t();
        float s = cv::determinant(U)*cv::determinant(Vt);

        float d1 = w.at<float>(0);
        float d2 = w.at<float>(1);
        float d3 = w.at<float>(2);
        if(d1/d2<1.00001 || d2/d3<1.00001)
        {
            return false;
        }
        std::vector<cv::Mat> vR, vt, vn;
        vR.reserve(8);
        vt.reserve(8);
        vn.reserve(8);

        //n'=[x1 0 x3] 4 posibilities e1=e3=1, e1=1 e3=-1, e1=-1 e3=1, e1=e3=-1
        float aux1 = sqrt((d1*d1-d2*d2)/(d1*d1-d3*d3));
        float aux3 = sqrt((d2*d2-d3*d3)/(d1*d1-d3*d3));
        float x1[] = {aux1,aux1,-aux1,-aux1};
        float x3[] = {aux3,-aux3,aux3,-aux3};
        //case d'=d2
        float aux_stheta = sqrt((d1*d1-d2*d2)*(d2*d2-d3*d3))/((d1+d3)*d2);

        float ctheta = (d2*d2+d1*d3)/((d1+d3)*d2);
        float stheta[] = {aux_stheta, -aux_stheta, -aux_stheta, aux_stheta};
        for(int i=0; i<4; i++)
        {
            cv::Mat Rp=cv::Mat::eye(3,3,CV_32F);
            Rp.at<float>(0,0)=ctheta;
            Rp.at<float>(0,2)=-stheta[i];
            Rp.at<float>(2,0)=stheta[i];
            Rp.at<float>(2,2)=ctheta;

            cv::Mat R = s*U*Rp*Vt;
            vR.push_back(R);

            cv::Mat tp(3,1,CV_32F);
            tp.at<float>(0)=x1[i];
            tp.at<float>(1)=0;
            tp.at<float>(2)=-x3[i];
            tp*=d1-d3;

            cv::Mat t = U*tp;
            vt.push_back(t/cv::norm(t));

            cv::Mat np(3,1,CV_32F);
            np.at<float>(0)=x1[i];
            np.at<float>(1)=0;
            np.at<float>(2)=x3[i];

            cv::Mat n = V*np;
            if(n.at<float>(2)<0)
                n=-n;
            vn.push_back(n);
        }

        //case d'=-d2
        float aux_sphi = sqrt((d1*d1-d2*d2)*(d2*d2-d3*d3))/((d1-d3)*d2);

        float cphi = (d1*d3-d2*d2)/((d1-d3)*d2);
        float sphi[] = {aux_sphi, -aux_sphi, -aux_sphi, aux_sphi};
        for(int i=0; i<4; i++)
        {
            cv::Mat Rp=cv::Mat::eye(3,3,CV_32F);
            Rp.at<float>(0,0)=cphi;
            Rp.at<float>(0,2)=sphi[i];
            Rp.at<float>(1,1)=-1;
            Rp.at<float>(2,0)=sphi[i];
            Rp.at<float>(2,2)=-cphi;

            cv::Mat R = s*U*Rp*Vt;
            vR.push_back(R);

            cv::Mat tp(3,1,CV_32F);
            tp.at<float>(0)=x1[i];
            tp.at<float>(1)=0;
            tp.at<float>(2)=x3[i];
            tp*=d1+d3;

            cv::Mat t = U*tp;
            vt.push_back(t/cv::norm(t));

            cv::Mat np(3,1,CV_32F);
            np.at<float>(0)=x1[i];
            np.at<float>(1)=0;
            np.at<float>(2)=x3[i];

            cv::Mat n = V*np;
            if(n.at<float>(2)<0)
                n=-n;
            vn.push_back(n);
        }


        int bestGood = 0;
        int secondBestGood = 0;    
        int bestSolutionIdx = -1;
        float bestParallax = -1;
        std::vector<cv::Point3f> bestP3D;
        std::vector<bool> bestTriangulated;
        // Instead of applying the visibility constraints proposed in the Faugeras' paper (which could fail for points seen with low parallax)
        // We reconstruct all hypotheses and check in terms of triangulated points and parallax
        for(size_t i=0; i<8; i++)
        {
            float parallaxi;
            std::vector<cv::Point3f> vP3Di;
            std::vector<bool> vbTriangulatedi;
            int nGood = CheckRT(vR[i],vt[i],mvKeys1,mvKeys2,mvMatches12,vbMatchesInliers,K,vP3Di, 4.0*mSigma*mSigma, vbTriangulatedi, parallaxi);

            if(nGood>bestGood)
            {
                secondBestGood = bestGood;
                bestGood = nGood;
                bestSolutionIdx = i;
                bestParallax = parallaxi;
                bestP3D = vP3Di;
                bestTriangulated = vbTriangulatedi;
            }
            else if(nGood>secondBestGood)
            {
                secondBestGood = nGood;
            }
        }

        if(secondBestGood<0.75*bestGood && bestParallax>=minParallax && bestGood>minTriangulated && bestGood>0.9*N)
        {
            vR[bestSolutionIdx].copyTo(R21);
            vt[bestSolutionIdx].copyTo(t21);
            vP3D = bestP3D;
            vbTriangulated = bestTriangulated;

            return true;
        }

        return false;
    }

    void Triangulate(const cv::KeyPoint &kp1, const cv::KeyPoint &kp2, const cv::Mat &P1, const cv::Mat &P2, cv::Mat &x3D)
    {
        cv::Mat A(4,4,CV_32F);

        A.row(0) = kp1.pt.x*P1.row(2)-P1.row(0);
        A.row(1) = kp1.pt.y*P1.row(2)-P1.row(1);
        A.row(2) = kp2.pt.x*P2.row(2)-P2.row(0);
        A.row(3) = kp2.pt.y*P2.row(2)-P2.row(1);

        cv::Mat u,w,vt;
        cv::SVD::compute(A,w,u,vt,cv::SVD::MODIFY_A| cv::SVD::FULL_UV);
        x3D = vt.row(3).t();
        x3D = x3D.rowRange(0,3)/x3D.at<float>(3);
    }

    void Normalize(const std::vector<cv::KeyPoint> &vKeys, std::vector<cv::Point2f> &vNormalizedPoints, cv::Mat &T)
    {
        float meanX = 0;
        float meanY = 0;
        const int N = vKeys.size();

        vNormalizedPoints.resize(N);

        for(int i=0; i<N; i++)
        {
            meanX += vKeys[i].pt.x;
            meanY += vKeys[i].pt.y;
        }

        meanX = meanX/N;
        meanY = meanY/N;

        float meanDevX = 0;
        float meanDevY = 0;

        for(int i=0; i<N; i++)
        {
            vNormalizedPoints[i].x = vKeys[i].pt.x - meanX;
            vNormalizedPoints[i].y = vKeys[i].pt.y - meanY;

            meanDevX += fabs(vNormalizedPoints[i].x);
            meanDevY += fabs(vNormalizedPoints[i].y);
        }

        meanDevX = meanDevX/N;
        meanDevY = meanDevY/N;

        float sX = 1.0/meanDevX;
        float sY = 1.0/meanDevY;

        for(int i=0; i<N; i++)
        {
            vNormalizedPoints[i].x = vNormalizedPoints[i].x * sX;
            vNormalizedPoints[i].y = vNormalizedPoints[i].y * sY;
        }

        T = cv::Mat::eye(3,3,CV_32F);
        T.at<float>(0,0) = sX;
        T.at<float>(1,1) = sY;
        T.at<float>(0,2) = -meanX*sX;
        T.at<float>(1,2) = -meanY*sY;
    }


    int CheckRT(const cv::Mat &R, const cv::Mat &t, const std::vector<cv::KeyPoint> &vKeys1, const std::vector<cv::KeyPoint> &vKeys2,
                        const std::vector<Match> &vMatches12, std::vector<bool> &vbMatchesInliers,
                        const cv::Mat &K, std::vector<cv::Point3f> &vP3D, float th2, std::vector<bool> &vbGood, float &parallax)
    {
        // Calibration parameters
        const float fx = K.at<float>(0,0);
        const float fy = K.at<float>(1,1);
        const float cx = K.at<float>(0,2);
        const float cy = K.at<float>(1,2);

        vbGood = std::vector<bool>(vKeys1.size(),false);
        vP3D.resize(vKeys1.size());

        std::vector<float> vCosParallax;
        vCosParallax.reserve(vKeys1.size());

        // Camera 1 Projection Matrix K[I|0]
        cv::Mat P1(3,4,CV_32F,cv::Scalar(0));
        K.copyTo(P1.rowRange(0,3).colRange(0,3));

        cv::Mat O1 = cv::Mat::zeros(3,1,CV_32F);

        // Camera 2 Projection Matrix K[R|t]
        cv::Mat P2(3,4,CV_32F);
        R.copyTo(P2.rowRange(0,3).colRange(0,3));
        t.copyTo(P2.rowRange(0,3).col(3));
        P2 = K*P2;

        cv::Mat O2 = -R.t()*t;

        int nGood=0;
        int stat[5]={0,0,0,0,0};
        //std::cout<<"vMatches12.size(): "<<vMatches12.size()<<std::endl;
        for(size_t i=0, iend=vMatches12.size();i<iend;i++)
        {
            
            if(!vbMatchesInliers[i])
                continue;

            const cv::KeyPoint &kp1 = vKeys1[vMatches12[i].first];
            const cv::KeyPoint &kp2 = vKeys2[vMatches12[i].second];
            cv::Mat p3dC1;

            Triangulate(kp1,kp2,P1,P2,p3dC1);

            if(!std::isfinite(p3dC1.at<float>(0)) || !std::isfinite(p3dC1.at<float>(1)) || !std::isfinite(p3dC1.at<float>(2)))
            {
                vbGood[vMatches12[i].first]=false;
                stat[0]++;
                continue;
            }

            // Check parallax
            cv::Mat normal1 = p3dC1 - O1;
            float dist1 = cv::norm(normal1);

            cv::Mat normal2 = p3dC1 - O2;
            float dist2 = cv::norm(normal2);

            float cosParallax = normal1.dot(normal2)/(dist1*dist2);

            // Check depth in front of first camera (only if enough parallax, as "infinite" points can easily go to negative depth)
            if(p3dC1.at<float>(2)<=0 && cosParallax<0.99998){
                stat[1]++;
                continue;
            }

            // Check depth in front of second camera (only if enough parallax, as "infinite" points can easily go to negative depth)
            cv::Mat p3dC2 = R*p3dC1+t;

            if(p3dC2.at<float>(2)<=0 && cosParallax<0.99998){
                stat[2]++;
                continue;
            }

            // Check reprojection error in first image
            float im1x, im1y;
            float invZ1 = 1.0/p3dC1.at<float>(2);
            im1x = fx*p3dC1.at<float>(0)*invZ1+cx;
            im1y = fy*p3dC1.at<float>(1)*invZ1+cy;

            float squareError1 = (im1x-kp1.pt.x)*(im1x-kp1.pt.x)+(im1y-kp1.pt.y)*(im1y-kp1.pt.y);

            if(squareError1>th2){
                //std::cout<<squareError1<<std::endl;
                stat[3]++;
                continue;
            }

            // Check reprojection error in second image
            float im2x, im2y;
            float invZ2 = 1.0/p3dC2.at<float>(2);
            im2x = fx*p3dC2.at<float>(0)*invZ2+cx;
            im2y = fy*p3dC2.at<float>(1)*invZ2+cy;

            float squareError2 = (im2x-kp2.pt.x)*(im2x-kp2.pt.x)+(im2y-kp2.pt.y)*(im2y-kp2.pt.y);

            if(squareError2>th2){
                //std::cout<<squareError1<<std::endl;
                stat[4]++;
                continue;
            }

            vCosParallax.push_back(cosParallax);
            vP3D[vMatches12[i].first] = cv::Point3f(p3dC1.at<float>(0),p3dC1.at<float>(1),p3dC1.at<float>(2));
            nGood++;

            if(cosParallax<0.99998)
                vbGood[vMatches12[i].first]=true;
        }

        if(nGood>0)
        {
            sort(vCosParallax.begin(),vCosParallax.end());

            size_t idx = std::min(50,int(vCosParallax.size()-1));
            parallax = acos(vCosParallax[idx])*180/CV_PI;
        }
        else
            parallax=0;

        //std::cout<<"3d creator filter sum: "<<stat[0]<<":"<<stat[1]<<":"<<stat[2]<<":"<<stat[3]<<":"<<stat[4]<<std::endl;
        return nGood;
    }

    void DecomposeE(const cv::Mat &E, cv::Mat &R1, cv::Mat &R2, cv::Mat &t)
    {
        cv::Mat u,w,vt;
        cv::SVD::compute(E,w,u,vt);

        u.col(2).copyTo(t);
        t=t/cv::norm(t);

        cv::Mat W(3,3,CV_32F,cv::Scalar(0));
        W.at<float>(0,1)=-1;
        W.at<float>(1,0)=1;
        W.at<float>(2,2)=1;

        R1 = u*W*vt;
        if(cv::determinant(R1)<0)
            R1=-R1;

        R2 = u*W.t()*vt;
        if(cv::determinant(R2)<0)
            R2=-R2;
    }
    
    bool MatchTwoFrame(std::vector<cv::KeyPoint>& kp1, std::vector<cv::KeyPoint>& kp2, 
            cv::Mat& desc1, cv::Mat& desc2, int width, int height,
            std::vector<std::vector<std::vector<std::size_t>>>& mGrid,
            cv::Mat cam_m, Eigen::Matrix3d& R, Eigen::Vector3d& t, cv::Mat debug_img){
        std::vector<cv::Point2f> vbPrevMatched;
        vbPrevMatched.resize(kp1.size());
        for(size_t i=0; i<kp1.size(); i++)
            vbPrevMatched[i]=kp1[i].pt;
        std::vector<int> vnMatches12;
        orb_slam::SearchForInitialization(kp1, kp2, desc1, desc2, 0.9, mGrid, 0, 0, width, height, true, vbPrevMatched, vnMatches12);
        
        cv::Mat R21;
        cv::Mat t21;
        cv::Mat outlier_cv;
        std::vector<bool> vbMatchesInliers;
        std::vector<std::pair<int, int>> mvMatches12;
        for(size_t i=0, iend=vnMatches12.size();i<iend; i++)
        {
            if(vnMatches12[i]>=0)
            {
                mvMatches12.push_back(std::make_pair(i,vnMatches12[i]));
            }
        }
        std::cout<<"mvMatches12: "<<mvMatches12.size()<<std::endl;
        orb_slam::FindRT(kp1, kp2, vbMatchesInliers, R21, t21, mvMatches12 , cam_m, debug_img);
        
        if(R21.cols==0){
            return false;
        }else{
            for (int i=0; i<3; i++){
                for (int j=0; j<3; j++){
                    R(i,j)=R21.at<float>(i,j);
                }
            }
            for (int i=0; i<3; i++){
                t(i,0)=t21.at<float>(i,0);
            }
            return true;
        }
    }
    
    
    bool PosInGrid(const cv::KeyPoint &kp, int &posX, int &posY, int mnMinX, int mnMinY, int mnMaxX, int mnMaxY)
    {
        float mfGridElementWidthInv=static_cast<float>(FRAME_GRID_COLS)/(mnMaxX-mnMinX);
        float mfGridElementHeightInv=static_cast<float>(FRAME_GRID_ROWS)/(mnMaxY-mnMinY);
        posX = round((kp.pt.x-mnMinX)*mfGridElementWidthInv);
        posY = round((kp.pt.y-mnMinY)*mfGridElementHeightInv);

        //Keypoint's coordinates are undistorted, which could cause to go out of the image
        if(posX<0 || posX>=FRAME_GRID_COLS || posY<0 || posY>=FRAME_GRID_ROWS)
            return false;

        return true;
    }
    
    void convert_eigen_opencv_mat(const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic>& matrix, cv::Mat& mat){
        mat = cv::Mat(matrix.rows(), matrix.cols(), CV_8UC1);
        for(int i=0; i<matrix.rows(); i++){
            for(int j=0; j<matrix.cols(); j++){
                mat.at<unsigned char>(i, j)=matrix(i, j);
            }
        }
        mat=mat.t();
    }

    void convert_eigen_opencv_kp(const Eigen::Matrix2Xd& kps, std::vector<cv::KeyPoint>& kps_cv){
        for(int i=0; i<kps.cols(); i++){
            cv::KeyPoint key;
            key.pt.x=kps(0,i);
            key.pt.y=kps(1,i);
            kps_cv.push_back(key);
        }
    }
    
    void ExtractOrb(std::string img_name, cv::Mat& desc_list, std::vector<cv::KeyPoint>& kps_list,
                    std::vector<std::vector<std::vector<std::size_t>>>& mGrid,
                    cv::Mat cam_m, cv::Mat cam_dis){
        cv::Mat img = cv::imread(img_name);
        cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
        ExtractOrb(img, desc_list, kps_list, mGrid, cam_m, cam_dis);
    }

    void ExtractOrb(cv::Mat img, cv::Mat& desc_list, std::vector<cv::KeyPoint>& kps_list,
                    std::vector<std::vector<std::vector<std::size_t>>>& mGrid,
                    cv::Mat cam_m, cv::Mat cam_dis){
        int features_count=2000;
        float features_scale_rate=1.2;
        int features_level=8;
        int ini_cell_fast=20;
        int min_cell_fast=7;
        cv::Mat img_undistort;
        cv::undistort(img, img_undistort, cam_m, cam_dis);
        float width_img=img_undistort.cols;
        float height_img=img_undistort.rows;
        
        orb_slam::ORBextractor extractor = orb_slam::ORBextractor(features_count, features_scale_rate, features_level, ini_cell_fast, min_cell_fast);
        extractor(img_undistort, kps_list, desc_list);

        int N=kps_list.size();
        int nReserve = 0.5f*N/(FRAME_GRID_COLS*FRAME_GRID_ROWS);
        mGrid.resize(FRAME_GRID_COLS);
        for(unsigned int i=0; i<FRAME_GRID_COLS;i++){
            mGrid[i].resize(FRAME_GRID_ROWS);
            for (unsigned int j=0; j<FRAME_GRID_ROWS;j++){
                mGrid[i][j].reserve(nReserve);
            }  
        }
        for(int i=0;i<N;i++)
        {
            const cv::KeyPoint &kp = kps_list[i];

            int nGridPosX, nGridPosY;
            if(PosInGrid(kp,nGridPosX,nGridPosY, 0, 0, width_img, height_img))
                mGrid[nGridPosX][nGridPosY].push_back(i);
        }
    }
}
