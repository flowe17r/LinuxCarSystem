#include <sys/types.h>                                                           
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <poll.h>
#include <pthread.h>

static void *thread_brightness_control(void *args)
{
    int fd = (int)args;
    unsigned char c;
    //int current_brightness;
    int delta;
    struct v4l2_queryctrl qctrl;
    memset(&qctrl,0, sizeof(struct v4l2_queryctrl));
    qctrl.id = V4L2_CID_BRIGHTNESS;
    if(ioctl(fd,VIDIOC_QUERYCTRL,&qctrl)!= 0)
    {
        printf("can not query brightness");
        return NULL;
    }
    printf("now brigthness minness%d maxness%d\n",qctrl.minimum,qctrl.maximum);
    delta = (qctrl.maximum -qctrl.minimum) / 10;

    struct v4l2_control ctl;
    ctl.id = V4L2_CID_BRIGHTNESS;
    ioctl(fd,VIDIOC_G_CTRL,&ctl);
    while(1)
    {
        c = getchar();
        if (c == 'u' || c=='U') 
        {
            ctl.value += delta;
        }
        else if (c=='d' ||c == 'D') 
        {
            ctl.value -= delta;
        }
        if (ctl.value > qctrl.maximum) 
        {
            ctl.value = qctrl.maximum;
        }
        if (ctl.value < qctrl.minimum) {
            ctl.value = qctrl.minimum;
        }                                                 

        ioctl(fd,VIDIOC_S_CTRL,&ctl);
    }
    return NULL;



}

int main(int argc,char **argv)
{
    int fd;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum fsenum;
    int fmt_index = 0;
    int frame_index = 0;
    void *bufs[32];
    int buf_cnt;
    char fielname[32];
    int file_cnt = 0;
    struct pollfd fds[1];

    if (argc != 2)
    {
        printf("Usage:%s <.../dev/videoX>,print format detail for video device\n",argv[0]);
        return -1;
    }
    fd = open(argv[1],O_RDWR);
    if (fd < 0)
    {
        printf("can not open %s\n", argv[1]);
        return -1;
    }
    //查询能力
    struct v4l2_capability capability;
    memset(&capability,0,sizeof(struct v4l2_capability));
    if (ioctl(fd,VIDIOC_QUERYCAP,&capability) == 0)
    {
        if((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
        {
            fprintf(stderr,"error opening device %s:video capture not supported\n",argv[1]);
            return -1;
        }
        if (!(capability.capabilities & V4L2_CAP_STREAMING))
        {
            fprintf(stderr,"%s not support streaming i/o \n",argv[1]);
            return -1;
        }
    }
    else
    {
        printf("can not get capability\n");
        return -1;
    }
    while (1)
    {
        //枚举帧格式
        
        fmtdesc.index = fmt_index;  // 比如从0开始
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;  // 指定type为"捕获"
        if(ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)!=0)
        {
            break;
        }
        frame_index =0;
        while (1)
        {
            //枚举出帧格式支持的帧大小
            memset(&fsenum, 0, sizeof(struct v4l2_frmsizeenum));
            fsenum.pixel_format  = fmtdesc.pixelformat;
            fsenum.index  = frame_index;
            if(ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&fsenum)==0)
            {
                 printf("format %s,%d, framesize %d: %d x %d\n", fmtdesc.description, fmtdesc.pixelformat, frame_index, fsenum.discrete.width, fsenum.discrete.height);
            }
            else
            {
                break;
            }
            frame_index++;
        }
        fmt_index++;
    }
    //设置格式
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //像素格式
    fmt.fmt.pix.field = V4L2_FIELD_NONE;//扫描模式：逐行扫描；
    if (ioctl(fd,VIDIOC_S_FMT,&fmt) == 0)
    {
        printf("set format ok : %d x %d \n",fmt.fmt.pix.width,fmt.fmt.pix.height);
    }
    else
    {
        printf("can not set format\n");
        return -1;
    }
    //申请buffer
    struct v4l2_requestbuffers rb;
    memset(&rb,0,sizeof(struct v4l2_requestbuffers));
    rb.count = 32;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd,VIDIOC_REQBUFS,&rb) == 0) 
    {
        //申请成功后，mmap这些buffer
        buf_cnt = rb.count;
        for (int i =0; i<rb.count;i++)
        {
            struct v4l2_buffer buf;
            memset(&buf,0,sizeof(struct v4l2_buffer));
            buf.index = i;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if (ioctl(fd,VIDIOC_QUERYBUF,&buf) ==0) 
            {
                //mmap
                bufs[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
                if (bufs[i] ==MAP_FAILED) 
                {
                    printf("Unable to map buffer\n");
                    return -1;
                }
            }
            else {
                printf("can not query buffer\n");
                return -1;
            }
        }
        printf("success mmap %d buffers \n",buf_cnt);
    }
    else 
    {
        printf("can not request buffers\n");
        return -1;
    }
    //把所有buffer放入空闲列表
    for (int i =0; i<rb.count;i++)
    {
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(struct v4l2_buffer));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd,VIDIOC_QBUF,&buf) !=0) 
        {
            perror("Unable to queue buffer");
            return -1;
        }
    }
    printf("queue buffers ok \n");
    //开启摄像头
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd,VIDIOC_STREAMON,&type)!=0) 
    {
        perror("Unable start capture");
        return -1;
    }
    printf("start capture ok \n");

    //创建新线程 使用u d控制亮度
    pthread_t thread;
    pthread_create(&thread,NULL,thread_brightness_control,(void*)fd);

    while (1)
    {
        //poll
        
        memset(fds, 0,sizeof(struct pollfd));
        fds[0].fd = fd;
        fds[0].events = POLLIN;   // 等待"可读"事件

        if(poll(fds, 1, -1) ==1)
        {
            //把buffer取出队列
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(struct v4l2_buffer));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                                              
            buf.memory = V4L2_MEMORY_MMAP;
            if (ioctl(fd,VIDIOC_DQBUF,&buf) != 0)
            {
                perror("Unable to dequeue buffer");
                return -1;
            }
             //把buffer的数据存为文件
            sprintf(fielname,"video_raw_data_%04d.raw",file_cnt++);
            int fd_file = open(fielname,O_RDWR | O_CREAT,0666);
            if (fd_file < 0) 
            {
                printf("can not create file :%s",fielname);
                return -1;
            }
            printf("capture to %s\n",fielname);
            write(fd_file,bufs[buf.index],buf.bytesused);
            close(fd_file);
            //把buffer放入队列
            if (ioctl(fd,VIDIOC_QBUF,&buf) !=0) 
            {
                perror("Unable to queue buffer");
                return -1;
            }
        }
    }
    if (ioctl(fd,VIDIOC_STREAMOFF,&type)!=0) 
    {
        perror("Unable stop capture");
        return -1;
    }
    printf("stop capture ok \n");
    close(fd);
    return 0;
}

