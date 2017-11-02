package xyz.eraise.libyuv.utils;

/**
 * Created by eraise on 2017/9/11.
 */

public class NdkBridge {

    static {
        System.loadLibrary("avcodec");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("fdk-aac");
        System.loadLibrary("postproc");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("yuvutil");
    }

    /**
     *
     * @param width
     * @param height
     * @param dstWidth
     * @param dstHeight
     * @param rotateModel 旋转角度,0 90 180 270
     * @param enableMirror
     */
    public static native void prepare(int width,
                                      int height,
                                      int dstWidth,
                                      int dstHeight,
                                      int rotateModel,
                                      boolean enableMirror,
                                      String videoBaseUrl,
                                      String videoName,
                                      String audioName);

    /**
     *
     * @param data
     */
    public static native void processVideo(byte[] data);

    public static native void processAudio(byte[] data);

    public static native void stop();
}