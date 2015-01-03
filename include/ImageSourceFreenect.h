#pragma once
#include <opencv2/opencv.hpp>

#include <QtCore/qmutex.h>
#include <libfreenect/libfreenect.hpp>

namespace iez_private {
	class ImageSourceFreenectDevice_private;
}
namespace iez {
class ImageSourceBase {
public:
	virtual cv::Mat getColorMat() = 0;
	virtual cv::Mat getDepthMat() = 0;
	virtual int getSequence() const = 0;

	virtual ~ImageSourceBase(){};
};


class ImageSourceFreenect:public ImageSourceBase
{
public:
	ImageSourceFreenect(const int index = 0);
	void streamInit(freenect_resolution resolution);
	cv::Mat getColorMat();
	cv::Mat getDepthMat();

	~ImageSourceFreenect() { freenect.deleteDevice(0); }
	int getSequence() const;
private:
	Freenect::Freenect freenect;
	iez_private::ImageSourceFreenectDevice_private *device;


};

}
