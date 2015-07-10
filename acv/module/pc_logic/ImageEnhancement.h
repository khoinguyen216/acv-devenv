#ifndef __IMAGE_ENHANCEMENT_WFL__
#define __IMAGE_ENHANCEMENT_WFL__

class ImageEnhancement
{
public:
	ImageEnhancement(int imageWidth, int imageHeight)
	{
		ime_alloc_global_buf(imageWidth, imageHeight);
	}
	ImageEnhancement():m_lum_plane(0), m_max_plane(0), m_integral_plane(0), m_param_plane(0)
	{
		// Nothing
	}
	int SetDimension(int imageWidth, int imageHeight)
	{
		if (m_lum_plane) ime_free_global_buf();
		return ime_alloc_global_buf(imageWidth, imageHeight);
	}
	~ImageEnhancement()
	{
		ime_free_global_buf();
	}
public:
	int Enhance(int* src_rgb, int src_stride,
					int* dst_rgb, int dst_stride,
					int width, int height,
					float strength);

private:
	
	int ime_preprocess(int* rgb_data, int stride, int w, int h);

	int ime_alloc_global_buf(int w, int h);
	void ime_free_global_buf();

	int _ime_calc_lum_max(int* rgb_data, int rgb_stride,
						unsigned char* lum_plane,
						unsigned char* max_plane,
						int w, int h);
	int _ime_calc_integral_plane(unsigned char* lum_plane, int* integral_plane, int w, int h);
	int _ime_calc_rect_sum(int* integral_plane, int w, int h, int top, int left, int rows, int cols);
	int _ime_calc_param(unsigned char* lum_plane, int* integral, int* param, int w, int h);

	int ime_enhance_sub_rect(int* src_rgb, int src_stride,
						 int* dst_rgb, int dst_stride,
						 int w, int h,
						 int sx, int ex,/*[sx, ex)*/
						 int sy, int ey,/*[sy, ey)*/
						 float strength);

	

private:
		/* 全局变量 */
	unsigned char* m_lum_plane;/*照度*/
	unsigned char* m_max_plane;/*调整上限*/
	int* m_integral_plane;/*积分图*/
	int* m_param_plane;/*参数*/
};

#endif