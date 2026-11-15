#pragma once
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace triangle {
using Vertices = std::array<cv::Point2f, 3>;
struct Detection { Vertices outer, inner; double area; };
struct Pose {
    cv::Vec3d rvec, tvec;
    std::vector<cv::Point2f> points, projected;
    double rms = std::numeric_limits<double>::infinity();
    double maxError = 0;
};

inline double cross(cv::Point2f a, cv::Point2f b) { return a.x*b.y-a.y*b.x; }

// 轮廓角点仅用于划分三条边；对每条边的中段像素拟合直线，交点作为最终顶点。
inline bool vertices(const std::vector<cv::Point>& contour, Vertices& result) {
    std::vector<cv::Point> hull, polygon;
    cv::convexHull(contour, hull);
    cv::approxPolyDP(hull, polygon, .025*cv::arcLength(hull, true), true);
    if (polygon.size() != 3) return false;
    cv::Point2f center;
    for (auto p : polygon) center += cv::Point2f(p);
    center *= 1.f/3;
    std::sort(polygon.begin(), polygon.end(), [center](auto a, auto b) {
        return std::atan2(a.y-center.y, a.x-center.x) < std::atan2(b.y-center.y, b.x-center.x);
    });
    std::array<cv::Vec4f, 3> lines;
    for (int i=0; i<3; ++i) {
        cv::Point2f a=polygon[i], v=cv::Point2f(polygon[(i+1)%3])-a;
        const double length=cv::norm(v);
        if (length<8) return false;
        std::vector<cv::Point2f> edge;
        for (auto pixel : contour) {
            cv::Point2f delta=cv::Point2f(pixel)-a;
            double along=delta.dot(v)/(length*length);
            if (along>.12 && along<.88 && std::abs(cross(v,delta))/length<std::max(2.,length*.025))
                edge.emplace_back(pixel);
        }
        if (edge.size()<5) return false;
        cv::fitLine(edge, lines[i], cv::DIST_HUBER, 0, .01, .01);
    }
    for (int i=0; i<3; ++i) {
        auto l=lines[(i+2)%3], r=lines[i];
        cv::Point2f a(l[2],l[3]), v(l[0],l[1]), b(r[2],r[3]), w(r[0],r[1]);
        double denom=cross(v,w);
        if (std::abs(denom)<.05) return false;
        result[i]=a+v*static_cast<float>(cross(b-a,w)/denom);
        if (cv::norm(result[i]-cv::Point2f(polygon[i]))>std::max(5.,cv::arcLength(hull,true)*.03))
            return false;
    }
    return true;
}

inline std::vector<Detection> detect(const cv::Mat& image) {
    cv::Mat gray, mask;
    if (image.channels()==1) gray=image; else cv::cvtColor(image,gray,cv::COLOR_BGR2GRAY);
    cv::threshold(gray,mask,0,255,cv::THRESH_BINARY_INV|cv::THRESH_OTSU);
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(mask,contours,hierarchy,cv::RETR_TREE,cv::CHAIN_APPROX_NONE);
    std::vector<Detection> found;
    for (size_t i=0;i<contours.size();++i) {
        double area=cv::contourArea(contours[i]);
        if (area<200 || area>image.total()*.9) continue;
        auto rect=cv::boundingRect(contours[i]);
        if (rect.x<2 || rect.y<2 || rect.br().x>=image.cols-2 || rect.br().y>=image.rows-2) continue;
        Vertices outer;
        if (!vertices(contours[i],outer)) continue;
        for (int child=hierarchy[i][2];child>=0;child=hierarchy[child][0]) {
            double ratio=cv::contourArea(contours[child])/area;
            if (ratio<.15 || ratio>.65) continue;
            Vertices inner;
            if (!vertices(contours[child],inner)) continue;
            std::vector<cv::Point2f> boundary(outer.begin(),outer.end());
            bool inside=true;
            for (auto p:inner) inside=inside && cv::pointPolygonTest(boundary,p,false)>0;
            if (inside) found.push_back({outer,inner,area});
        }
    }
    std::sort(found.begin(),found.end(),[](const auto& a,const auto& b){return a.area>b.area;});
    return found;
}

// 直接使用实测内外边长，两等边三角形同心同向，原点为公共重心。
inline std::vector<cv::Point3f> model(double side, double innerSide) {
    if (!std::isfinite(side) || !std::isfinite(innerSide) || innerSide<=0 || side<=innerSide)
        throw std::runtime_error("Need 0 < inner side < outer side (meters)");
    const float h=static_cast<float>(side*std::sqrt(3.)/2);
    Vertices ideal={cv::Point2f(0,-2*h/3),cv::Point2f(side/2,h/3),cv::Point2f(-side/2,h/3)};
    std::vector<cv::Point3f> points;
    for (auto p:ideal) points.emplace_back(p.x,p.y,0);
    for (auto p:ideal) {
        p*=static_cast<float>(innerSide/side);
        points.emplace_back(p.x,p.y,0);
    }
    return points;
}

inline bool evaluate(const std::vector<cv::Point3f>& objects,const std::vector<cv::Point2f>& points,
                     const cv::Mat& k,const cv::Mat& d,const cv::Vec3d& r,const cv::Vec3d& t,Pose& result) {
    if (!std::isfinite(cv::norm(t)) || !std::isfinite(cv::norm(r)) || t[2]<=0) return false;
    cv::Matx33d rotation; cv::Rodrigues(r,rotation);
    for (auto p:objects) if ((rotation*cv::Vec3d(p.x,p.y,p.z)+t)[2]<=0) return false;
    result.rvec=r; result.tvec=t; result.points=points;
    cv::projectPoints(objects,r,t,k,d,result.projected);
    double sum=0; result.maxError=0;
    for (size_t i=0;i<points.size();++i) {
        double e=cv::norm(points[i]-result.projected[i]);
        sum+=e*e; result.maxError=std::max(result.maxError,e);
    }
    result.rms=std::sqrt(sum/points.size());
    return std::isfinite(result.rms);
}

inline bool solve(const Detection& target,const std::vector<cv::Point3f>& objects,
                  const cv::Mat& k,const cv::Mat& d,Pose& best) {
    best=Pose{};
    // 枚举外点的循环对应与内点的循环对应，避免依据画面“最上方”猜物体朝向。
    // 不枚举镜像；纸张正面朝相机。对称图形不能唯一确定绕法线的 120 度朝向。
    for (int outerShift=0;outerShift<3;++outerShift) for (int innerShift=0;innerShift<3;++innerShift) {
        std::vector<cv::Point2f> points;
        for (int i=0;i<3;++i) points.push_back(target.outer[(i+outerShift)%3]);
        for (int i=0;i<3;++i) points.push_back(target.inner[(i+innerShift)%3]);
        std::vector<cv::Mat> rotations,translations;
        cv::solvePnPGeneric(objects,points,k,d,rotations,translations,false,cv::SOLVEPNP_IPPE);
        for (size_t j=0;j<rotations.size();++j) {
            cv::Vec3d r=rotations[j],t=translations[j];
            Pose candidate;
            if (!evaluate(objects,points,k,d,r,t,candidate)) continue;
            cv::solvePnPRefineLM(objects,points,k,d,r,t);
            Pose refined;
            if (evaluate(objects,points,k,d,r,t,refined) && refined.rms<candidate.rms) candidate=refined;
            if (candidate.rms<best.rms) best=candidate;
        }
    }
    return std::isfinite(best.rms);
}

inline void draw(cv::Mat& image,const Detection& detection,const Pose* pose=nullptr) {
    for (auto vertices:{detection.outer,detection.inner}) for (int i=0;i<3;++i) {
        cv::line(image,vertices[i],vertices[(i+1)%3],cv::Scalar(0,200,0),2);
        cv::circle(image,vertices[i],4,cv::Scalar(0,0,255),-1);
    }
    if (pose) {
        for (auto p:pose->projected) cv::circle(image,p,6,cv::Scalar(255,0,255),1);
        std::string text=cv::format("Z %.3fm  range %.3fm  RMS %.2fpx",pose->tvec[2],cv::norm(pose->tvec),pose->rms);
        cv::putText(image,text,{12,28},cv::FONT_HERSHEY_SIMPLEX,.6,cv::Scalar(0,0,255),2);
    }
}
} // namespace triangle
