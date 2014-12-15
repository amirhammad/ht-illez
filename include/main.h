//#pragma once
//
//#include <QtGui>
//#include "ImageSource.h"
//#include "Processing.h"
//namespace iez {
//class CMainWindow : public QMainWindow {
//
//public:
//	CMainWindow(): QMainWindow()//,kinect(0),processing(0)
//
//	{
//		QWidget *w = new QWidget();
//		this->setCentralWidget(w);
//		show();
//
//
//	}
//
//	void init() {
//		using namespace std;
//		int err;
//		err = kinect.init();
//		if (err) {
//			std::cerr<<"CANNOT OPEN KINECT\n";
//			return;
//		}
//		processing.init();
//	}
//	~CMainWindow()
//	{
//	}
//
//
//private:
//	CImageSource kinect;
//	CProcessing processing;
////	class __CMyThead: public QThread {
////		__CMyThead(QWidget *parent = 0):QThread(parent)
////		{
////
////		}
////
////		void run()
////		{
////			process1();
////		}
////		void process1();
////	} m_thread;
//
//
//};
//
//}
