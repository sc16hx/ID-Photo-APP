#include "seetaface_JniClient.h"
#include "CMImgProc.h"
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <android/bitmap.h>
#include <jni.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/opencv.hpp>

#include "FaceDetection/include/common.h"

#include "FaceDetection/include/face_detection.h"

#include "globalmatting.h"
#include "guidedfilter.h"

using namespace cv;
using namespace std;
using namespace seeta;
#define  LOG_TAG    "opencvProcess"
#define ALOGD(...) \
            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define ALOGE(...) \
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);
#define ALOGV(...) \
            __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__);

#define ASSERT(status, ret)     if (!(status)) { return ret; }
#define ASSERT_FALSE(status)    ASSERT(status, false)

extern "C"{
    JNIEXPORT jstring JNICALL
    Java_com_ghnor_idmaker_HomeActivity_stringFromJNI(
            JNIEnv *env,
            jobject /* this */) {
        std::string hello = "Welcome to ID Photo Maker!!!";
        return env->NewStringUTF(hello.c_str());
    }

    JNIEXPORT jintArray JNICALL
    Java_com_ghnor_idmaker_OpenCVActivity_gray(JNIEnv *env, jobject instance, jintArray buf, jint w,
                                               jint h) {
        jint *cbuf = env->GetIntArrayElements(buf, JNI_FALSE );
        if (cbuf == NULL) {
            return 0;
        }

        cv::Mat imgData(h, w, CV_8UC4, (unsigned char *) cbuf);

        uchar* ptr = imgData.ptr(0);
        for(int i = 0; i < w*h; i ++){
            //计算公式：Y(亮度) = 0.299*R + 0.587*G + 0.114*B
            //对于一个int四字节，其彩色值存储方式为：BGRA
            int grayScale = (int)(ptr[4*i+2]*0.299 + ptr[4*i+1]*0.587 + ptr[4*i+0]*0.114);
            ptr[4*i+1] = grayScale;
            ptr[4*i+2] = grayScale;
            ptr[4*i+0] = grayScale;
        }

        int size = w * h;
        jintArray result = env->NewIntArray(size);
        env->SetIntArrayRegion(result, 0, size, cbuf);
        env->ReleaseIntArrayElements(buf, cbuf, 0);
        return result;
    }


    void BitmapToMat2(JNIEnv *env, jobject& bitmap, Mat& mat, jboolean needUnPremultiplyAlpha) {
        AndroidBitmapInfo info;
        void *pixels = 0;
        Mat &dst = mat;

        try {
            CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
            CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                      info.format == ANDROID_BITMAP_FORMAT_RGB_565);
            CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
            CV_Assert(pixels);
            dst.create(info.height, info.width, CV_8UC4);
            if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
                Mat tmp(info.height, info.width, CV_8UC4, pixels);
                if (needUnPremultiplyAlpha) cvtColor(tmp, dst, COLOR_mRGBA2RGBA);
                else tmp.copyTo(dst);
            } else {
    //             info.format == ANDROID_BITMAP_FORMAT_RGB_565
                Mat tmp(info.height, info.width, CV_8UC2, pixels);
                cvtColor(tmp, dst, COLOR_BGR5652RGBA);
            }
            AndroidBitmap_unlockPixels(env, bitmap);
            return;
        } catch (const cv::Exception &e) {
            AndroidBitmap_unlockPixels(env, bitmap);
            jclass je = env->FindClass("org/opencv/core/CvException");
            if (!je) je = env->FindClass("java/lang/Exception");
            env->ThrowNew(je, e.what());
            return;
        } catch (...) {
            AndroidBitmap_unlockPixels(env, bitmap);
            jclass je = env->FindClass("java/lang/Exception");
            env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
            return;
        }
    }

    void BitmapToMat(JNIEnv *env, jobject& bitmap, Mat& mat) {
        BitmapToMat2(env, bitmap, mat, false);
    }

    void MatToBitmap2
            (JNIEnv *env, Mat& mat, jobject& bitmap, jboolean needPremultiplyAlpha) {
        AndroidBitmapInfo info;
        void *pixels = 0;
        Mat &src = mat;
        ALOGD("aaa-cols=%d, rows=%d",src.cols, src.rows);
        try {
            CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);

            CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                      info.format == ANDROID_BITMAP_FORMAT_RGB_565);
            CV_Assert(src.dims == 2 && info.height == (uint32_t) src.rows &&
                      info.width == (uint32_t) src.cols);
            CV_Assert(src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4);
            CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
            CV_Assert(pixels);
            if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
                Mat tmp(info.height, info.width, CV_8UC4, pixels);
                if (src.type() == CV_8UC1) {
                    cvtColor(src, tmp, COLOR_GRAY2RGBA);
                } else if (src.type() == CV_8UC3) {
                    cvtColor(src, tmp, COLOR_RGB2RGBA);
                } else if (src.type() == CV_8UC4) {
                    if (needPremultiplyAlpha)
                        cvtColor(src, tmp, COLOR_RGBA2mRGBA);
                    else
                        src.copyTo(tmp);
                }
            } else {
                // info.format == ANDROID_BITMAP_FORMAT_RGB_565
                Mat tmp(info.height, info.width, CV_8UC2, pixels);
                if (src.type() == CV_8UC1) {
                    cvtColor(src, tmp, COLOR_GRAY2BGR565);
                } else if (src.type() == CV_8UC3) {
                    cvtColor(src, tmp, COLOR_RGB2BGR565);
                } else if (src.type() == CV_8UC4) {
                    cvtColor(src, tmp, COLOR_RGBA2BGR565);
                }
            }
            AndroidBitmap_unlockPixels(env, bitmap);
            return;
        } catch (const cv::Exception &e) {
            AndroidBitmap_unlockPixels(env, bitmap);
            jclass je = env->FindClass("org/opencv/core/CvException");
            if (!je) je = env->FindClass("java/lang/Exception");
            env->ThrowNew(je, e.what());
            return;
        } catch (...) {
            AndroidBitmap_unlockPixels(env, bitmap);
            jclass je = env->FindClass("java/lang/Exception");
            env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
            return;
        }
    }

    void MatToBitmap(JNIEnv *env, Mat& mat, jobject& bitmap) {
        MatToBitmap2(env, mat, bitmap, true);
    }

    JNIEXPORT int JNICALL Java_com_ghnor_idmaker_CameraActivity_Resize(JNIEnv *env,jobject instance, jobject bitmap, jobject bitmap2){
            Mat img;
            int flag;
            //bitmap转化成mat
            BitmapToMat(env,bitmap,img);

            string tDetectModelPath = "/sdcard/seeta_fd_frontal_v1.0.bin";
            seeta::FaceDetection detector(tDetectModelPath.c_str());

        	detector.SetMinFaceSize(40);
        	detector.SetScoreThresh(2.f);
        	detector.SetImagePyramidScaleFactor(0.8f);
        	detector.SetWindowStep(4, 4);

        	cv::Mat imgmini;
        	cv::Rect mini;
        	mini.x = img.cols*0.1;
        	mini.y = img.rows*0.1;
        	mini.height = img.rows*0.6;
        	mini.width = img.cols*0.6;
        	cv::Mat temps = img.clone();

        	img = img(mini);

        	cv::Mat img_gray;

        	if (img.channels() != 1)
        		cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);
        	else
        		img_gray = img;

        	cv::Mat centerROI, centerGray, cutROI, reImg;
        	cv::Rect centerRect, cutRect;

        	seeta::ImageData img_data;
        	img_data.data = img_gray.data;
        	img_data.width = img_gray.cols;
        	img_data.height = img_gray.rows;
        	img_data.num_channels = 1;

        	cv::Rect face_rect;
        	std::vector<seeta::FaceInfo> faces = detector.Detect(img_data);
        	int32_t num_face = static_cast<int32_t>(faces.size());

        	if (num_face != 0) {
        		flag = 1;
        		for (int32_t i = 0; i < 1; i++) {
        		    int face_y =  mini.y + faces[i].bbox.y - faces[i].bbox.width*0.75;
        		    int face_x = mini.x + faces[i].bbox.x - faces[i].bbox.width*0.4375;
        		    int face_height = faces[i].bbox.width*2.5;
        		    //face_rect.x = mini.x + faces[i].bbox.x - faces[i].bbox.width*0.4375;
                    //face_rect.y = mini.y + faces[i].bbox.y - faces[i].bbox.width*0.75;
        		    //face_rect.height = faces[i].bbox.width*2.5;
        		    face_rect.width = faces[i].bbox.width*1.875;
                    cv::Mat result(face_rect.height, face_rect.width, CV_8UC3, cv::Scalar(255, 255, 255));


        		    if(face_y<0 && face_x>0){
                        face_rect.x = face_x;
                        face_rect.y = 0;
                        face_rect.height = face_height + face_y;
                        result = temps(face_rect);
                        int tmp = 0 - face_y;
                        cv::copyMakeBorder(result,result,tmp,0,0,0,cv::BORDER_CONSTANT,cv::Scalar(0, 255, 0));

                        cv::Size reSize = cv::Size(300, 400);
                        cv::resize(result, reImg, reSize);
                        //MatToBitmap(env,reImg,bitmap2);
                        cv::Mat background(400,300, CV_8UC3,cv::Scalar(0, 255, 0));
                        ALOGD("back-cols=%d, rows=%d",background.cols, background.rows);
                        ALOGD("reImg-cols=%d, rows=%d",reImg.cols, reImg.rows);
                        //bitwise_or(reImg,background, reImg);
                        reImg.copyTo(background);
                        reImg = background.clone();
                        //MatToBitmap(env,reImg,bitmap2);
        		    }else if((face_x<0 && face_x<0)||face_x+face_rect.width>temps.cols||face_y+face_height>temps.rows){
        		        ALOGD("Invalid Image!!");
                        return 0;
        		    }else{
        		        face_rect.x = face_x;
        		        face_rect.y = face_y;
                        face_rect.height = face_height;
                        result = temps(face_rect);
                        cv::Size reSize = cv::Size(300, 400);
                        cv::resize(result, reImg, reSize);
        		    }
        			ALOGD("x=%d, y=%d, width=%d, height=%d",face_rect.x,face_rect.y,face_rect.width,face_rect.height);
        		}
        	}
        	else
        	{
        		flag = 0;
        		return 1;
        	}
            cvtColor(reImg, reImg,COLOR_BGRA2BGR);
            ALOGD("reImg2-cols=%d, rows=%d",reImg.cols, reImg.rows);
            MatToBitmap(env,reImg,bitmap2);
            return 2;
        }

    JNIEXPORT void JNICALL Java_com_ghnor_idmaker_CameraActivity_Background(JNIEnv *env,jobject instance, jobject bitmap, jobject bitmap2, jint color){
        Mat reImg;
        BitmapToMat(env,bitmap,reImg);
        cvtColor(reImg, reImg,COLOR_BGRA2BGR);
        //MatToBitmap(env,reImg,bitmap2);
        cv::Rect rectangle(2, 40, reImg.cols-5, reImg.rows-40);
                                             	cv::Mat result;
                                             	cv::Mat bgModel, fgModel;

                                             	//grabCut()最后一个参数为cv::GC_INIT_WITH_MASK时
                                             	result = cv::Mat(reImg.rows, reImg.cols, CV_8UC1, cv::Scalar(cv::GC_BGD));
                                             	cv::Mat roi(result, rectangle);
                                             	roi = cv::Scalar(cv::GC_PR_FGD);
                                             	//这两步可以合并（此处体现了使用bgModel , fgModel的价值）
                                             	cv::grabCut(reImg, result, rectangle, bgModel, fgModel, 1,
                                             		cv::GC_INIT_WITH_MASK);
                                             	cv::grabCut(reImg, result, rectangle, bgModel, fgModel, 3,
                                             		cv::GC_INIT_WITH_MASK);


                                             	cv::compare(result, cv::GC_PR_FGD, result, cv::CMP_EQ);
                                             	//result = result & 1 ;
                                             	cv::Mat foreground(reImg.size(), CV_8UC3,
                                             		cv::Scalar(255, 255, 255));

                                             	//cv::imshow("Mask", result);
                                             	reImg.copyTo(foreground, result);
                                             	//MatToBitmap(env,result,bitmap2);

                                                cv::Mat srcGray;
                                             	cvtColor(foreground, srcGray, CV_BGR2GRAY);
                                             	//imshow("srcGray", srcGray);
                                             	cv::Mat thresh;
                                             	threshold(srcGray, thresh, 230, 255, CV_THRESH_BINARY_INV);
                                             	//imshow("thresh", thresh);
                                             	//Custom core
                                             	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 13));
                                             	//open operation
                                             	cv::Mat open_result;
                                             	cv::morphologyEx(thresh, open_result, cv::MORPH_OPEN, element);
                                             	//imshow("open_result", open_result);
                                             	//close operation
                                             	cv::Mat close_result;
                                             	cv::morphologyEx(thresh, close_result, cv::MORPH_CLOSE, element);
                                             	//imshow("close_result", close_result);
                                             	//Morphological gradient
                                             	//dilate
                                             	cv::Mat dilate_result;
                                             	dilate(close_result, dilate_result, element);
                                             	//imshow("dilate_result", dilate_result);
                                             	//erode
                                             	cv::Mat erode_result;
                                             	erode(close_result, erode_result, element);
                                             	//imshow("erode_result", erode_result);
                                             	cv::Mat outd;
                                             	cv::Mat interme(dilate_result.size(),CV_8UC4);
                                             	//xor to get the gap between erode result and dilate result
                                             	bitwise_xor(dilate_result, erode_result, interme);
                                             	int height = interme.rows;//height
                                             	int width = interme.cols;//width
                                             	//for all pixels
                                             	for (int row = 0; row < height; row++) {
                                             		for (int col = 0; col < width; col++) {
                                             			//int gray = gray_src.at<uchar>(row, col);
                                             			if (interme.at<uchar>(row, col) >128)
                                             				interme.at<uchar>(row, col) = 128;
                                             		}
                                             	}

                                             	cv::Mat out;
                                             	cv::Mat trimap(reImg.size(), CV_8UC4);
                                             	bitwise_or(interme,erode_result, trimap);
                                             	//imshow("trimap", trimap);
                                             	//MatToBitmap(env,trimap,bitmap2);
                                                cv::Mat fore, alpha;
                                             	globalMatting(reImg, trimap, fore, alpha);

                                             	// filter the result with fast guided filter
                                             	alpha = guidedFilter(reImg, alpha, 9, 1e-5);
                                             	for (int x = 0; x < trimap.cols; ++x)
                                             		for (int y = 0; y < trimap.rows; ++y)
                                             		{
                                             			if (trimap.at<uchar>(y, x) == 0)
                                             				alpha.at<uchar>(y, x) = 0;
                                             			else if (trimap.at<uchar>(y, x) == 255)
                                             				alpha.at<uchar>(y, x) = 255;
                                             		}
                                             	//MatToBitmap(env,alpha,bitmap2);
                                             	cv::Mat background;
                                                cv::Mat result3(reImg.size(), reImg.type());
                                                if(color == 1){ // white
                                                                                        cv::Mat color(reImg.size(), CV_8UC3, cv::Scalar(255, 255, 255));
                                                                                        background = color.clone();
                                                                                    }
                                                                                    else if(color == 2){ // Red
                                                                                        cv::Mat color(reImg.size(), CV_8UC3, cv::Scalar(255, 0, 0));
                                                                                        background = color.clone();
                                                                                    }
                                                                                    else if(color == 2){ // Blue
                                                                                        cv::Mat color(reImg.size(), CV_8UC3, cv::Scalar(0, 0, 255));
                                                                                        background = color.clone();
                                                                                    }
                                                                                    else if(color == 0){
                                                                                        MatToBitmap(env,reImg,bitmap2);
                                                                                        return;
                                                                                    }

                                             	//cv::Mat background(image.size(), CV_8UC3,cv::Scalar(0, 0, 255));
                                             	double w = 0.0;
                                             	int b = 0, g = 0, r = 0;
                                             	int b1 = 0, g1 = 0, r1 = 0;
                                             	int b2 = 0, g2 = 0, r2 = 0;

                                             	for (int i = 0; i < height; i++) {
                                             		for (int j = 0; j < width; j++) {
                                             			//int m = alpha.at<uchar>(i, j);
                                             			int m = alpha.at<uchar>(i, j);
                                             			if (m == 255) {
                                             				result3.at<cv::Vec3b>(i, j) = reImg.at<cv::Vec3b>(i, j);
                                             				//Assign the foreground in the original image to the foreground in the resulting image
                                             			}
                                             			else if (m == 0) {
                                             				result3.at<cv::Vec3b>(i, j) = background.at<cv::Vec3b>(i, j);
                                             				//let the background be the assigened color (red here)
                                             			}
                                             			else {
                                             				w = m / 255.0;//weight
                                             				//Edge foreground
                                             				b1 = reImg.at<cv::Vec3b>(i, j)[0];
                                             				g1 = reImg.at<cv::Vec3b>(i, j)[1];
                                             				r1 = reImg.at<cv::Vec3b>(i, j)[2];
                                             				//Edge background
                                             				b2 = background.at<cv::Vec3b>(i, j)[0];
                                             				g2 = background.at<cv::Vec3b>(i, j)[1];
                                             				r2 = background.at<cv::Vec3b>(i, j)[2];
                                             				//Edge fusion
                                             				b = b1 * w + b2 * (1.0 - w);
                                             				g = g1 * w + g2 * (1.0 - w);
                                             				r = r1 * w + r2 * (1.0 - w);
                                             				result3.at<cv::Vec3b>(i, j)[0] = b;
                                             				result3.at<cv::Vec3b>(i, j)[1] = g;
                                             				result3.at<cv::Vec3b>(i, j)[2] = r;
                                             			}
                                             		}
                                             	}
                                             	MatToBitmap(env,result3,bitmap2);
    }
}