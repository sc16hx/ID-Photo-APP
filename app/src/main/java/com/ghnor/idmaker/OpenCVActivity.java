package com.ghnor.idmaker;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.ImageView;
import java.lang.NullPointerException;
import java.util.Timer;

import java.util.TimerTask;
import android.os.Handler;

public class OpenCVActivity extends AppCompatActivity {
    private Timer mTimer;
    private TimerTask mTimerTask;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private ImageView img;
    /*
    final Handler handler = new Handler();
    private Runnable runnable = new Runnable() {
        public void run() {
            Bitmap bitmap = BitmapFactory.decodeResource(getApplicationContext().getResources(), R.mipmap.ai);

            int w = bitmap.getWidth(), h = bitmap.getHeight();
            int[] pix = new int[w * h];
            bitmap.getPixels(pix, 0, w, 0, 0, w, h);
            // 调用JNI实现的gray方法
            int [] resultPixes = gray(pix,w,h);
            //String tRetStr = detect(pix,w,h,"/sdcard/");
            //System.out.printf("have face?? %s \n",tRetStr);
            //int [] resultPixes = detect(pix,w,h,"/sdcard/");
            //byte [] resultPixes = detect(pix,w,h,"/sdcard/");
            //Bitmap result = BitmapFactory.decodeByteArray(resultPixes, 0, resultPixes.length);
            Bitmap result = Bitmap.createBitmap(w,h, Bitmap.Config.RGB_565);
            result.setPixels(resultPixes, 0, w, 0, 0,w, h);
            //Bitmap result = Bitmap.createBitmap(300,400, Bitmap.Config.RGB_565);
            //result.setPixels(resultPixes, 0, 300, 0, 0,300, 400);
            ImageView img = (ImageView)findViewById(R.id.img2);
            //img.setImageBitmap(result);
            Bitmap bitmap2 = Bitmap.createBitmap(300,400, Bitmap.Config.RGB_565);
            detectFace(bitmap,"/sdcard/",bitmap2);
            img.setImageBitmap(bitmap2);
            //this.update();
            handler.postDelayed(this, 1000 * 120);// 间隔120秒
            // }
            /*
            void update(){
                //刷新msg的内容
            }

        }
    };
    */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.cv_test);

        Bitmap bitmap = BitmapFactory.decodeResource(this.getApplicationContext().getResources(), R.mipmap.ai);

        int w = bitmap.getWidth(), h = bitmap.getHeight();
        int[] pix = new int[w * h];
        bitmap.getPixels(pix, 0, w, 0, 0, w, h);
        // 调用JNI实现的gray方法
        int [] resultPixes = gray(pix,w,h);
        //String tRetStr = detect(pix,w,h,"/sdcard/");
        //System.out.printf("have face?? %s \n",tRetStr);
        //int [] resultPixes = detect(pix,w,h,"/sdcard/");
        //byte [] resultPixes = detect(pix,w,h,"/sdcard/");
        //Bitmap result = BitmapFactory.decodeByteArray(resultPixes, 0, resultPixes.length);
        Bitmap result = Bitmap.createBitmap(w,h, Bitmap.Config.RGB_565);
        result.setPixels(resultPixes, 0, w, 0, 0,w, h);
        //Bitmap result = Bitmap.createBitmap(300,400, Bitmap.Config.RGB_565);
        //result.setPixels(resultPixes, 0, 300, 0, 0,300, 400);
        ImageView img = (ImageView)findViewById(R.id.img2);
        //img.setImageBitmap(result);
        Bitmap bitmap3 = BitmapFactory.decodeResource(this.getApplicationContext().getResources(), R.mipmap.ai);
        Bitmap bitmap2 = Bitmap.createBitmap(300,400, Bitmap.Config.RGB_565);
        Resize(bitmap3,bitmap2);
        /*
        Bitmap bitmap2 = Bitmap.createBitmap(w,h, Bitmap.Config.RGB_565);
        bitmap2.setPixels(resultPixes, 0, 300, 0, 0,300, 400);
        bitmap2 = detectFace(bitmap,"/sdcard/",bitmap2);
        */
        img.setImageBitmap(bitmap2);
        //handler.postDelayed(runnable, 1000 * 60);
    }
    /*
    @Override
    protected void onDestroy() {
        handler.removeCallbacks(runnable); //停止刷新
        super.onDestroy();
    }
*/
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native int[] gray(int[] buf, int w, int h);

    public native Bitmap detectFace(Bitmap bitmap, String vFaceModelPath,Bitmap outBitmap);

    public native String detect(int[] buf, int w,int h, String vFaceModelPath);

    public native String stringFromJNI();

    public native void Canny(Object bitmap);
    public native void Resize(Object bitmap,Object bitmap2);
}
