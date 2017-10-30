package xyz.eraise.libyuv;

import android.Manifest;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.tbruyelle.rxpermissions2.RxPermissions;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.MessageFormat;
import java.util.List;

import io.reactivex.functions.Consumer;
import xyz.eraise.libyuv.utils.YuvUtil;

public class MainActivity extends AppCompatActivity
        implements SurfaceHolder.Callback, Camera.PreviewCallback {

    private Camera mCamera;
    private AudioRecord mAudioRecord;

    private SurfaceView mPreview;
    private Button btnTake;

    private static final int OUT_WIDTH = 540;
    private static final int OUT_HEIGHT = 480;

    private static final String TAG = "MainActivity";

    private boolean isInitEncode = false;
    private boolean isStop = false;
    private boolean isRecord = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mPreview = (SurfaceView) findViewById(R.id.surface);
        btnTake = (Button) findViewById(R.id.btn_take);
        btnTake.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                takePhoto();
            }
        });

        SurfaceHolder holder = mPreview.getHolder();
        holder.addCallback(this);

        int sampleRateHz = 44100;
        int channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int bufferSizeInBytes = AudioRecord.getMinBufferSize(sampleRateHz,
                channelConfig,
                audioFormat);
        mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
                sampleRateHz,
                channelConfig,
                audioFormat,
                bufferSizeInBytes);
        mAudioRecord.setRecordPositionUpdateListener(new AudioRecord.OnRecordPositionUpdateListener() {
            @Override
            public void onMarkerReached(AudioRecord recorder) {

            }

            @Override
            public void onPeriodicNotification(AudioRecord recorder) {

            }
        });
    }

    private void openCamera() {
        new RxPermissions(this)
                .request(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                .subscribe(new Consumer<Boolean>() {
                    @Override
                    public void accept(Boolean grant) throws Exception {
                        if (grant) {
                            /* 摄像头参数设置 */
                            mCamera = Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK);
                            mCamera.setDisplayOrientation(90);
                            Camera.Parameters parameters = mCamera.getParameters();
//                            parameters.setPictureSize(800, 480);
                            parameters.setPreviewFormat(ImageFormat.YV12);

                            List<Camera.Size> previewSizes = parameters.getSupportedPreviewSizes();
                            Camera.Size previewSize = null;
                            Camera.Size tempSize;
                            for (int i = 0; i < previewSizes.size(); i ++) {
                                tempSize = previewSizes.get(i);
                                Log.i(TAG, MessageFormat.format("preview width:{0, number}, height:{1, number}",
                                        tempSize.width,
                                        tempSize.height));
                                if ((float)tempSize.height / (float)tempSize.width == 480f/800f) {
                                    previewSize = tempSize;
                                    if (previewSize.width == 800) {
                                        break;
                                    }
                                }
                            }
                            if (previewSize == null) {
                                previewSize = previewSizes.get(0);
                            }
                            Log.i(TAG, MessageFormat.format("last show preview width:{0, number}, height:{1, number}",
                                    previewSize.width,
                                    previewSize.height));
                            parameters.setPreviewSize(previewSize.width, previewSize.height);
                            mCamera.setPreviewCallback(MainActivity.this);
                            mCamera.setParameters(parameters);
                            Log.i(TAG, MessageFormat.format("preview format:{0, number}", parameters.getPreviewFormat()));

                            try {
                                mCamera.setPreviewDisplay(mPreview.getHolder());
                            } catch (IOException e) {
                                e.printStackTrace();
                            }

                            Log.i(TAG, "init encode");
                            isInitEncode = true;
                        }
                    }
                });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCamera != null)
            mCamera.stopPreview();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        openCamera();
        if (mCamera != null)
            mCamera.startPreview();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void takePhoto() {
        if (isRecord) {
            isStop = true;
            isRecord = false;
            YuvUtil.stop();
            Toast.makeText(this, "结束", Toast.LENGTH_SHORT).show();
            btnTake.setText("录制");
        } else {
            startRecord();
            btnTake.setText("停止");
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (!isRecord)
            return;
        if (!isInitEncode)
            return;
//        Log.i(TAG, "preview");
        if (isStop)
            return;
//        long start = System.currentTimeMillis();
        processFrame(data, camera);
//        Log.i(TAG, MessageFormat.format("耗时：{0, number} ms", (System.currentTimeMillis() - start)));
    }

    private void startRecord() {
        /* 确保文件不存在 */
        File outputFile = new File(getOutputMediaDir(), "last.h264");
        if (outputFile.exists()) {
            outputFile.delete();
        }

        /* 初始化录制工具 */
//      pamera.Size previewSize = parameters.getPreviewSize();
        Camera.Size previewSize = mCamera.getParameters().getPreviewSize();
        YuvUtil.prepare(previewSize.width,
                previewSize.height,
                OUT_WIDTH,
                OUT_HEIGHT,
                90,
                false,
                getOutputMediaDir(),
                "last.h264");

        isRecord = true;
        isStop = false;
    }

    private void processFrame(byte[] data, Camera camera) {
        long timestamp = System.currentTimeMillis();
        // 报像头获取的图片帧传送到 ndk 层交由 ffmpeg 进行编码保存
        YuvUtil.process(data);
        Log.i(TAG, MessageFormat.format("yuv处理费时：{0, number} ms", System.currentTimeMillis() - timestamp));

    }

    // save image to sdcard path: Pictures/MyTestImage/
    public void saveImageData(byte[] imageData) {
        File imageFile = getOutputMediaFile();
        if (imageFile == null) {
            return;
        }
        try {
            FileOutputStream fos = new FileOutputStream(imageFile, false);
            fos.write(imageData);
            fos.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            Log.e(TAG, "File not found: " + e.getMessage());
        } catch (IOException e) {
            e.printStackTrace();
            Log.e(TAG, "Error accessing file: " + e.getMessage());
        }
    }

    public static String getOutputMediaDir() {
        File dir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES), "MyYuvImage");
        if (!dir.exists()) {
            if (!dir.mkdirs()) {
                Log.e(MainActivity.TAG, "can't makedir for imagefile");
                return null;
            }
        };
        return dir.getAbsolutePath() + File.separator;
    }

    public static File getOutputMediaFile() {
        File imageFileDir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES), "MyYuvImage");
        if (!imageFileDir.exists()) {
            if (!imageFileDir.mkdirs()) {
                Log.e(MainActivity.TAG, "can't makedir for imagefile");
                return null;
            }
        }
        // Create a media file name
//        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        String timeStamp = "LAST";
        File imageFile = new File(imageFileDir.getPath() + File.separator +
                "IMG_" + timeStamp + ".yuv");
        Log.i(TAG, MessageFormat.format("图片保存于：{0}", imageFile.getAbsolutePath()));
        return imageFile;
    }

    private void testPrintBitmapSize(byte[] data, Camera camera) {
        //这里直接用 BitmapFactory.decodeByteArray()是不行的.
        Camera.Parameters parameters = camera.getParameters();
        int imageFormat = parameters.getPreviewFormat(); //拿到默认格式
        int w = parameters.getPreviewSize().width; //拿到预览尺寸
        int h = parameters.getPreviewSize().height;
        Rect rect = new Rect(0, 0, w, h);
        Log.d(TAG, String.format("imageFormat:%d;size:%d,%d", imageFormat, w, h));
        YuvImage yuvImg = new YuvImage(data, imageFormat, w, h, null);
        try {
            ByteArrayOutputStream os = new ByteArrayOutputStream();
            yuvImg.compressToJpeg(rect, 100, os);
            Bitmap bitmap = BitmapFactory.decodeByteArray(os.toByteArray(), 0, os.size());
            Log.d(TAG, String.format("bitmap size:%d,%d", bitmap.getWidth(), bitmap.getHeight()));
        } catch (Exception e) {
        }
    }
}
