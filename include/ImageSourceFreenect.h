#pragma once
#include <opencv2/opencv.hpp>

#include <QtCore/qmutex.h>
#include <libfreenect/libfreenect.hpp>
#include "ImageSource.h"

namespace iez_private {
	class ImageSourceFreenectDevice_private;
}


namespace iez {
class ImageSourceFreenect:public ImageSourceBase
{
public:
	ImageSourceFreenect(const int index = 0);
	void streamInit(freenect_resolution resolution);
	cv::Mat getColorMat() const;
	cv::Mat getDepthMat() const;

	~ImageSourceFreenect() { freenect.deleteDevice(0); }
private:
	Freenect::Freenect freenect;
	iez_private::ImageSourceFreenectDevice_private *device;


};

}
