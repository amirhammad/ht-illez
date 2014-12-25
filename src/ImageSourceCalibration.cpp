#include "ImageSourceCalibration.h"


using namespace iez;
using namespace cv;

#define fx_rgb 5.2921508098293293e+02
#define fy_rgb 5.2556393630057437e+02
#define cx_rgb 3.2894272028759258e+02
#define cy_rgb 2.6748068171871557e+02
#define k1_rgb 2.6451622333009589e-01
#define k2_rgb -8.3990749424620825e-01
#define p1_rgb -1.9922302173693159e-03
#define p2_rgb 1.4371995932897616e-03
#define k3_rgb 9.1192465078713847e-01

#define fx_d 5.9421434211923247e+02
#define fy_d 5.9104053696870778e+02
#define cx_d 3.3930780975300314e+02
#define cy_d 2.4273913761751615e+02
#define k1_d -2.6386489753128833e-01
#define k2_d 9.9966832163729757e-01
#define p1_d -7.6275862143610667e-04
#define p2_d 5.0350940090814270e-03
#define k3_d -1.3053628089976321e+00

ImageSourceCalibration::ImageSourceCalibration(void)
{
	
}


ImageSourceCalibration::~ImageSourceCalibration(void)
{

}


void ImageSourceCalibration::calibratePixel(const Mat depth, int y, int x, int *rgb_x, int *rgb_y)
{
	Point3_<uint16_t> ret;
	uint16_t d = depth.at<uint16_t>(y,x);
	
}

static float raw_depth_to_meters(int raw_depth)
{
  if (raw_depth < 2047)
  {
   return 1.0 / (raw_depth * -0.0030711016 + 3.3309495161);
  }
  return 0;
}

void ImageSourceCalibration::calibrate(Mat &calibratedImage ,const Mat &depth, const Mat &image)
{
	int y;
	int x;
	
	for (y = 0; y < depth.rows; y++) {
		for (x = 0; x < depth.cols; x++) {
			double d = raw_depth_to_meters(depth.at<uint16_t>(y,x));
			int rgb_x = (x-cx_d)*d/fx_d;
			int rgb_y = (y-cy_d)*d/fy_d;
			if ((rgb_x >= 0 && rgb_x < image.cols) &&
					(rgb_y >= 0 && rgb_y <image.rows) ) {
				calibratedImage.at<Point3_<uint8_t> >(y,x) = image.at<Point3_<uint8_t> >(rgb_y, rgb_x);
			}
		}
	}
}
