#pragma once
#include <opencv2/opencv.hpp>

#include <QtCore/qmutex.h>
#include <libfreenect/libfreenect.hpp>

namespace iez_private {
	class ImageSourceFreenectDevice_private;
}
namespace iez {

class ImageSourceFreenect
{
public:
	ImageSourceFreenect(const int index = 0);
	cv::Mat getColorMat();
	cv::Mat getDepthMat();
	void streamInit(freenect_resolution resolution);
	~ImageSourceFreenect() { freenect.deleteDevice(0); }
private:
	Freenect::Freenect freenect;
	iez_private::ImageSourceFreenectDevice_private *device;


};

}
