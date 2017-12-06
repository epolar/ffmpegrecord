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
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.tbruyelle.rxpermissions2.RxPermissions;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;

import io.reactivex.Observable;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.disposables.Disposable;
import io.reactivex.functions.Consumer;
import io.reactivex.schedulers.Schedulers;
import xyz.eraise.libyuv.utils.NdkBridge;

public class MainActivity extends AppCompatActivity
        implements SurfaceHolder.Callback, Camera.PreviewCallback {

    private Camera mCamera;
    private AudioRecord mAudioRecord;

    private SurfaceView mPreview;
    private Button btnTake;
    private TextView tvDuration;

    private static final int OUT_WIDTH = 540;
    private static final int OUT_HEIGHT = 480;

    private static final String TAG = "MainActivity";

    private boolean isInitEncode = false;
    private boolean isRecord = false;

    private int mBufferSizeInBytes;

    private volatile byte[] lastFrame;

    private Timer mTimer;
    private TimerTask mDurationTask;
    private long mDuration;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mPreview = (SurfaceView) findViewById(R.id.surface);
        btnTake = (Button) findViewById(R.id.btn_take);
        tvDuration = (TextView) findViewById(R.id.tv_duration);
        btnTake.setOnClickListener(v -> takePhoto());
        findViewById(R.id.guideline_video).setOnClickListener(v -> autoFocus());

        SurfaceHolder holder = mPreview.getHolder();
        holder.addCallback(this);

        int channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int sampleRateHz = getRecordRate(channelConfig, audioFormat);
        mBufferSizeInBytes = AudioRecord.getMinBufferSize(sampleRateHz,
                channelConfig,
                audioFormat);
        int bytesPerFrame = 4096;
        mBufferSizeInBytes += (bytesPerFrame - mBufferSizeInBytes % bytesPerFrame);
		/* Get number of samples. Calculate the buffer size
		 * (round up to the factor of given frame size)
		 * 使能被整除，方便下面的周期性通知
		 * */
//        int frameSize = mBufferSizeInBytes / bytesPerFrame;
//        int frame_count = 160;
//        if (frameSize % frame_count != 0) {
//            frameSize += (frame_count - frameSize % frame_count);
//            mBufferSizeInBytes = frameSize * bytesPerFrame;
//        }
        mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
                sampleRateHz,
                channelConfig,
                audioFormat,
                mBufferSizeInBytes);
//        mAudioRecord.setPositionNotificationPeriod(frame_count);
    }

    public int getRecordRate(int channelConfig, int audioFormat) {
//        int[] rateList = new int[] {8000, 11025, 16000, 22050,
//                32000, 37800, 44056, 44100, 47250, 4800, 50000, 50400, 88200,
//                96000, 176400, 192000, 352800, 2822400, 5644800};
        int[] rateList = new int[] {44100, 22050, 32000, 11025, 8000, 37800, 44056,
                47250, 4800, 50000, 50400, 88200, 96000,
                176400, 192000, 352800, 2822400, 5644800};
        for (int rate : rateList) {  // add the rates you wish to check against
            int bufferSize = AudioRecord.getMinBufferSize(rate, channelConfig, audioFormat);
            if (bufferSize != AudioRecord.ERROR
                    && bufferSize != AudioRecord.ERROR_BAD_VALUE && bufferSize > 0) {
                // buffer size is valid, Sample rate supported
                return rate;
            }
        }
        return 44100;
    }

    private void openCamera() {
        new RxPermissions(this)
                .request(Manifest.permission.CAMERA)
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

                            List<String> focusModeList = parameters.getSupportedFocusModes();
                            if (focusModeList.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
                                Camera.Area area = new Camera.Area(new Rect(-100, -100, 100, 100), 1000);
                                List<Camera.Area> focusAreas = new ArrayList<>();
                                focusAreas.add(area);
                                parameters.setFocusAreas(focusAreas);
                                parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                            }

                            List<int[]> supportFpsRange = parameters.getSupportedPreviewFpsRange();
                            int[] fpsRange = supportFpsRange.get(0);
                            parameters.setPreviewFpsRange(fpsRange[0], fpsRange[1]);

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
                        if (mCamera != null)
                            mCamera.startPreview();
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
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void takePhoto() {
        if (isRecord) {
            isRecord = false;
            if (mDurationTask != null) {
                mDurationTask.cancel();
                mDurationTask = null;
                mTimer.purge();
            }
//            NdkBridge.stop();
//            Toast.makeText(this, "结束", Toast.LENGTH_SHORT).show();
//            btnTake.setText("录制");
        } else {
            new RxPermissions(MainActivity.this)
                    .request(Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    .subscribe(grant -> {
                        if (grant) {
                            startRecord();
                            btnTake.setText("停止");
                        }
                    });
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (!isRecord)
            return;
        if (!isInitEncode)
            return;
//        Log.i(TAG, "preview");
//        long start = System.currentTimeMillis();
        lastFrame = data;
//        Log.i(TAG, MessageFormat.format("耗时：{0, number} ms", (System.currentTimeMillis() - start)));
    }

    private void autoFocus() {
        mCamera.autoFocus(new Camera.AutoFocusCallback() {
            @Override
            public void onAutoFocus(boolean success, Camera camera) {
                if (success) {
                    Log.d(TAG, "focus success");
                } else {
                    Log.d(TAG, "focus fail");
                }
            }
        });
    }

    private void startRecord() {
        String videoFileName = "last.h264";
        String audioFileName = "last.aac";
        /* 确保文件不存在 */
        File videoFile = new File(getOutputMediaDir(), videoFileName);
        if (videoFile.exists()) {
            videoFile.delete();
        }
        File audioFile = new File(getOutputMediaDir(), audioFileName);
        if (audioFile.exists()) {
            audioFile.delete();
        }

        /* 初始化录制工具 */
//      pamera.Size previewSize = parameters.getPreviewSize();
        Camera.Size previewSize = mCamera.getParameters().getPreviewSize();
        NdkBridge.prepare(mAudioRecord.getSampleRate(),
                previewSize.width,
                previewSize.height,
                OUT_WIDTH,
                OUT_HEIGHT,
                90,
                false,
                getOutputMediaDir(),
                videoFileName,
                audioFileName);

        isRecord = true;

        new Thread(this::recordAudio).start();
        Observable.create(subscription-> {
            while (isRecord) {
                if (lastFrame != null)
                    processFrame(lastFrame.clone(), null);
            }
            NdkBridge.stop();
            subscription.onNext(true);
            subscription.onComplete();
        }).subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(obj -> {
                    Toast.makeText(MainActivity.this, "结束", Toast.LENGTH_SHORT).show();
                    btnTake.setText("录制");
                });

        if (mTimer == null) {
            mTimer = new Timer();
        }
        if (mDurationTask != null) {
            mDurationTask.cancel();
        }
        mDuration = 0;
        mDurationTask = new TimerTask() {
            @Override
            public void run() {
                mDuration += 10;
                runOnUiThread(() ->
                        tvDuration.setText(MessageFormat.format("{0, number, 0.00}", mDuration / 1000f)));
            }
        };
        mTimer.schedule(mDurationTask, 10, 10);

    }

    private void recordAudio() {
        mAudioRecord.startRecording();
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
        int bufSize = NdkBridge.getAudioFrameSize();
        byte[] buf = new byte[bufSize];
        while (isRecord) {
            int len = mAudioRecord.read(buf, 0, bufSize);
            if (len > 0) {
                NdkBridge.processAudio(len, buf);
            }
        }
        mAudioRecord.stop();
    }

    private void processFrame(byte[] data, Camera camera) {
        // 报像头获取的图片帧传送到 ndk 层交由 ffmpeg 进行编码保存
        NdkBridge.processVideo(data);
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
