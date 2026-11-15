#include "TriangleTarget.hpp"
#include <filesystem>
#include <iostream>

int main(int argc,char** argv) {
    try {
        const cv::String keys=
            "{help h||}{detect-only||Only detect, no pose}"
            "{image||Camera photograph (empty: live camera)}"
            "{template|" TRIANGLE_TEMPLATE "|Print source image}"
            "{params|" TRIANGLE_CALIBRATION "|Calibration YAML}"
            "{camera|/dev/v4l/by-id/usb-HJ_USB_2.0_Camera_HJ_USB_2.0_Camera_SN0001-video-index0|Camera device}"
            "{side|0.096|Outer side length in meters}"
            "{inner-side|0.056|Inner side length in meters}"
            "{distance|0|Measured slant distance to outer-triangle centroid, meters}"
            "{output||Annotated output photograph}";
        cv::CommandLineParser parser(argc,argv,keys);
        if (parser.has("help")) {parser.printMessage();return 0;}
        const auto sourcePath=parser.get<std::string>("template");
        const auto imagePath=parser.get<std::string>("image");
        const auto outputPath=parser.get<std::string>("output");
        const double side=parser.get<double>("side"),measured=parser.get<double>("distance");
        const double innerSide=parser.get<double>("inner-side");
        const bool detectOnly=parser.has("detect-only");
        if (!parser.check()) {parser.printErrors();return 1;}
        if (!std::isfinite(side) || side<=0 || !std::isfinite(measured) || measured<0)
            throw std::runtime_error("side must be positive; distance must be nonnegative (meters)");
        auto source=cv::imread(sourcePath);
        if (source.empty()) throw std::runtime_error("Cannot load print template: "+sourcePath);
        if (triangle::detect(source).empty()) throw std::runtime_error("No hollow triangle in template");
        auto objects=triangle::model(side,innerSide);
        std::cout<<"外三边各 "<<side*100<<" cm，高 "<<side*std::sqrt(3.)/2*100
                 <<" cm；内三边各 "<<innerSide*100<<" cm；原点为公共重心（内外同心同向）。\n";
        cv::Mat k,d; cv::Size calibrated;
        if (!detectOnly) {
            if (!imagePath.empty() && std::filesystem::equivalent(imagePath,sourcePath))
                throw std::runtime_error("打印源图不是相机照片；只能用 --detect-only 检查，不能据此测距。");
            const auto calibrationPath=parser.get<std::string>("params");
            cv::FileStorage fs(calibrationPath,cv::FileStorage::READ);
            if (!fs.isOpened()) throw std::runtime_error("Cannot load calibration: "+calibrationPath);
            fs["camera_matrix"]>>k; fs["dist_coeffs"]>>d;
            fs["image_width"]>>calibrated.width; fs["image_height"]>>calibrated.height;
            if (k.rows!=3 || k.cols!=3 || !cv::checkRange(k) || !cv::checkRange(d) ||
                (d.total()!=4 && d.total()!=5 && d.total()!=8 && d.total()!=12 && d.total()!=14) ||
                calibrated.width<=0 || calibrated.height<=0)
                throw std::runtime_error("标定文件必须包含有效 K、畸变及 image_width/image_height；请使用最新 ChArUco YAML。");
            k.convertTo(k,CV_64F); d.convertTo(d,CV_64F);
            if (k.at<double>(0,0)<=0 || k.at<double>(1,1)<=0) throw std::runtime_error("Invalid focal lengths");
            std::cout<<"标定文件: "<<calibrationPath<<"\n分辨率: "<<calibrated
                     <<"，fx/fy="<<k.at<double>(0,0)/k.at<double>(1,1)<<"\n";
            std::cout<<"仅适用于同一相机、焦距及成像模式；不自动拉伸或缩放输入图片。\n";
        }
        auto process=[&](const cv::Mat& frame,cv::Mat& out,bool report) {
            if (!detectOnly && frame.size()!=calibrated)
                throw std::runtime_error("图片尺寸与标定不一致；请用同一相机的标定分辨率拍摄，不要强制拉伸。");
            auto targets=triangle::detect(frame); out=frame.clone();
            bool solved=false; triangle::Pose best; triangle::Detection selected;
            // 多目标时只显示图像中面积最大的匹配靶标，避免无意切到很小的噪声。
            if (!targets.empty()) {
                selected=targets.front();
                if (!detectOnly) solved=triangle::solve(selected,objects,k,d,best);
                triangle::draw(out,selected,solved?&best:nullptr);
            }
            if (targets.empty())
                cv::putText(out,"No hollow triangle: show all 6 corners",{12,28},
                            cv::FONT_HERSHEY_SIMPLEX,.55,cv::Scalar(0,0,255),2);
            else if (!detectOnly && !solved)
                cv::putText(out,"Triangle found, pose unavailable",{12,28},
                            cv::FONT_HERSHEY_SIMPLEX,.55,cv::Scalar(0,0,255),2);
            if (report) {
                std::cout<<"检测到空心三角形: "<<targets.size()<<"\n";
                if (detectOnly && !targets.empty()) std::cout<<"已定位外、内轮廓的 6 个顶点；未计算距离。\n";
                if (solved) {
                    double range=cv::norm(best.tvec);
                    std::cout<<"X(右+)="<<best.tvec[0]<<" m，Y(下+)="<<best.tvec[1]
                             <<" m，Z(光轴深度)="<<best.tvec[2]<<" m\n"
                             <<"到三角形重心的直线距离="<<range<<" m\n"
                             <<"6点重投影 RMS="<<best.rms<<" px，最大误差="<<best.maxError<<" px\n";
                    if (measured>0) std::cout<<"实测直线距离="<<measured<<" m，误差="<<(range-measured)*100
                                             <<" cm，绝对相对误差="<<std::abs(range-measured)/measured*100<<"%\n";
                    std::cout<<"低重投影误差不等于标定正确；请在不同距离、画面中心/边缘复测。\n"
                             <<"等边三角形存在120度对称及平面位姿歧义，不报告唯一朝向。\n";
                }
            }
            return detectOnly?!targets.empty():solved;
        };
        if (!imagePath.empty()) {
            auto frame=cv::imread(imagePath);
            if (frame.empty()) throw std::runtime_error("Cannot load image: "+imagePath);
            cv::Mat out; bool success=process(frame,out,true);
            if (!outputPath.empty()) {
                if (std::filesystem::exists(outputPath)) throw std::runtime_error("Output already exists; use a new output filename");
                if (!cv::imwrite(outputPath,out)) throw std::runtime_error("Cannot write annotated image");
                std::cout<<"标注图: "<<outputPath<<"\n";
            }
            return success?0:2;
        }
        cv::VideoCapture cap(parser.get<std::string>("camera"),cv::CAP_V4L2);
        if (!cap.isOpened()) throw std::runtime_error("Cannot open camera; select correct --camera device");
        if (!detectOnly) {
            cap.set(cv::CAP_PROP_FRAME_WIDTH,calibrated.width);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT,calibrated.height);
        }
        std::cout<<"把打印靶标放入画面，按 p 打印当前结果，q/Esc 退出。\n";
        cv::Mat frame,out; bool report=false;
        while (cap.read(frame) && !frame.empty()) {
            process(frame,out,report); report=false;
            cv::imshow("Triangle PnP: p=print, q=quit",out);
            int key=cv::waitKey(20)&255;
            if (key=='q' || key==27) break;
            if (key=='p') report=true;
        }
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
    return 0;
}
