//
// Created by eraise on 2017/9/29.
//

class Arguments
{
public :
    int in_width, in_height, out_width, out_height, rotate_model;
    bool enable_mirror;
    char *base_url, *video_name, *audio_name;
    long start_time;
    int video_bit_rate = 1546;
    int video_fps = 20;
    int audio_sample_rate = 44100;
};