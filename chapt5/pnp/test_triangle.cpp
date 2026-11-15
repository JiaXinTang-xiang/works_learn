#include "TriangleTarget.hpp"
#include <iostream>

static void require(bool value,const char* message) {
    if (!value) throw std::runtime_error(message);
}

int main(int argc,char** argv) {
    try {
        require(argc==2,"Need print-source test image");
        cv::Mat source=cv::imread(argv[1]);
        auto objects=triangle::model(.096,.056);
        for (int i=0;i<3;++i)
            require(std::abs(cv::norm(objects[i]-objects[(i+1)%3])-.096)<1e-6,
                    "Outer triangle must have three 9.6cm sides");
        for (int i=0;i<3;++i)
            require(std::abs(cv::norm(objects[3+i]-objects[3+(i+1)%3])-.056)<1e-6,
                    "Inner triangle must have three 5.6cm sides");
        auto templates=triangle::detect(source);
        require(templates.size()==1,"Must detect exactly one template target");
        const cv::Mat k=(cv::Mat_<double>(3,3)<<510,0,320,0,508,240,0,0,1);
        const cv::Mat distortion=(cv::Mat_<double>(1,5)<<.06,-.1,0,-.001,.03);
        // 带径向畸变的精确投影：验证三角形点对应、正深度、米制尺度。
        for (int i=0;i<5;++i) {
            cv::Vec3d r(.12*i,.08*i,.5*i),t(.015*i,-.01*i,.35+.1*i);
            std::vector<cv::Point2f> projected;
            cv::projectPoints(objects,r,t,k,distortion,projected);
            triangle::Detection detection;
            for (int j=0;j<3;++j) {
                detection.outer[j]=projected[(j+i)%3];
                detection.inner[j]=projected[3+(j+2*i)%3];
            }
            triangle::Pose pose;
            require(triangle::solve(detection,objects,k,distortion,pose),"PnP failed");
            require(cv::norm(pose.tvec-t)<.0002,"Metric translation recovery error too high");
            require(pose.rms<.01,"Exact projection must have low residual");
            std::cout<<"projection "<<i<<": translation error="<<cv::norm(pose.tvec-t)<<" m\n";
        }
        // 实际栅格轮廓流水线：透视投影源图、检测边缘交点、解算距离。
        auto templateTarget=templates.front();
        std::vector<cv::Point2f> templatePoints;
        for (auto p:templateTarget.outer) templatePoints.push_back(p);
        // model() 将最上方点作为第一个顶点；检测排序一般相同，显式旋转确保一致。
        int top=0;
        for (int j=1;j<3;++j) if (templatePoints[j].y<templatePoints[top].y) top=j;
        std::rotate(templatePoints.begin(),templatePoints.begin()+top,templatePoints.end());
        for (auto p:templateTarget.inner) templatePoints.push_back(p);
        const cv::Mat noDistortion=cv::Mat::zeros(1,5,CV_64F);
        for (int i=0;i<4;++i) {
            cv::Vec3d r(.15*i,-.12*i,.6*i),t(.03*i,-.015*i,.38+.08*i);
            std::vector<cv::Point2f> projected;
            cv::projectPoints(objects,r,t,k,noDistortion,projected);
            // 用外三点 + 内重心四个不共线点恢复准确平面变换。
            std::vector<cv::Point2f> from(templatePoints.begin(),templatePoints.begin()+3);
            std::vector<cv::Point2f> to(projected.begin(),projected.begin()+3);
            cv::Point2f sourceCenter,projectedCenter;
            for (int j=0;j<3;++j) {sourceCenter+=templateTarget.inner[j];projectedCenter+=projected[3+j];}
            from.push_back(sourceCenter/3);to.push_back(projectedCenter/3);
            cv::Mat h=cv::getPerspectiveTransform(from,to),rendered;
            cv::warpPerspective(source,rendered,h,{640,480},cv::INTER_LINEAR,cv::BORDER_CONSTANT,cv::Scalar(255,255,255));
            auto found=triangle::detect(rendered);
            require(!found.empty(),"Cannot detect rendered triangle");
            triangle::Pose pose;
            require(triangle::solve(found.front(),objects,k,noDistortion,pose),"Rendered PnP failed");
            double relative=cv::norm(pose.tvec-t)/cv::norm(t);
            require(relative<.04,"Rendered metric error exceeds 4 percent");
            std::cout<<"raster "<<i<<": relative translation error="<<relative*100<<"%, RMS="<<pose.rms<<"px\n";
        }
        cv::Mat blank(480,640,CV_8UC3,cv::Scalar(255,255,255));
        require(triangle::detect(blank).empty(),"Blank frame must not be a target");
        std::cout<<"Synthetic tests only: not independent evidence of real calibration accuracy.\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
    return 0;
}
