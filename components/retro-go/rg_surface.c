#include "rg_system.h"
#include "rg_surface.h"
#include "lodepng.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define CHECK_SURFACE(surface, retval)                                                                     \
    if (!surface || !surface->data)                                                                        \
    {                                                                                                      \
        RG_LOGE("Invalid surface data");                                                                   \
        return retval;                                                                                     \
    }                                                                                                      \
    else if (surface->width < 1 || surface->width > 4096 || surface->height < 1 || surface->height > 4096) \
    {                                                                                                      \
        RG_LOGE("Invalid surface dimensions %d %d", surface->width, surface->height);                      \
        return retval;                                                                                     \
    }                                                                                                      \
    else if ((surface->format & RG_PIXEL_FORMAT) == 0)                                                     \
    {                                                                                                      \
        RG_LOGE("Invalid surface format %d", surface->format);                                             \
        return retval;                                                                                     \
    }

rg_surface_t *rg_surface_create(int width, int height, int format, uint32_t alloc_flags)
{
    size_t palette = (format & RG_PIXEL_PALETTE) ? (256 * (format == RG_PIXEL_PAL888 ? 3 : 2)) : 0;
    size_t stride = width * RG_PIXEL_GET_SIZE(format);
    size_t size = sizeof(rg_surface_t) + (height * stride) + palette;
    rg_surface_t *surface = alloc_flags ? rg_alloc(size, alloc_flags) : malloc(size);
    if (!surface)
    {
        RG_LOGE("Surface allocation failed!");
        return NULL;
    }
    surface->width = width;
    surface->height = height;
    surface->stride = stride;
    surface->format = format;
    surface->data = (void *)surface + sizeof(rg_surface_t);
    surface->palette = palette ? ((void *)surface + size - palette) : NULL;
    return surface;
}

void rg_surface_free(rg_surface_t *surface)
{
    free(surface);
}

bool rg_surface_copy(const rg_surface_t *source, const rg_rect_t *source_rect, rg_surface_t *dest,
                     const rg_rect_t *dest_rect, bool scale)
{
    CHECK_SURFACE(source, false);
    CHECK_SURFACE(dest, false);

    int copy_width = dest->width;
    int copy_height = dest->height;
    int transparency = -1;

    // This function will eventually replace rg_gui_copy_buffer and rg_gui_draw_image but not today!

    if (source->width == copy_width && source->height == copy_height)
        scale = false;

    if (!scale && transparency == -1 && dest->stride == source->stride && dest->format == source->format)
    {
        memcpy(dest->data, source->data, dest->height * dest->stride);
    }
    else
    {
        float step_x = (float)source->width / copy_width;
        float step_y = (float)source->height / copy_height;

        // This may look weird but it avoids up to 75k branches and float multiplications...
        // Maybe there's a better way, without making it even more macro-heavy...
        short src_x_map[copy_width];
        for (int x = 0; x < copy_width; ++x)
            src_x_map[x] = scale ? (int)(x * step_x) : x;

        #define COPY_PIXELS_1(SRC_PIXEL, DST_PIXEL)                           \
            for (int y = 0; y < copy_height; ++y)                             \
            {                                                                 \
                int src_y = scale ? (y * step_y) : y;                         \
                const uint8_t *src = source->data + (src_y * source->stride); \
                uint8_t *dst = dest->data + (y * dest->stride);               \
                for (int x = 0; x < copy_width; ++x)                          \
                {                                                             \
                    int src_x = (int)src_x_map[x];                            \
                    uint16_t pixel = SRC_PIXEL;                               \
                    if ((int)pixel == transparency)                           \
                        continue;                                             \
                    DST_PIXEL;                                                \
                }                                                             \
            }

        #define COPY_PIXELS(SRC_PIXEL)                                                                   \
            if (dest->format == RG_PIXEL_565_LE)                                                         \
            {                                                                                            \
                COPY_PIXELS_1(SRC_PIXEL, ((uint16_t *)dst)[x] = pixel);                                  \
            }                                                                                            \
            else if (dest->format == RG_PIXEL_565_BE)                                                    \
            {                                                                                            \
                COPY_PIXELS_1(SRC_PIXEL, ((uint16_t *)dst)[x] = (pixel << 8) | (pixel >> 8));            \
            }                                                                                            \
            else if (dest->format == RG_PIXEL_888)                                                       \
            {                                                                                            \
                COPY_PIXELS_1(SRC_PIXEL, *dst++ = ((pixel >> 8) & 0xF8); *dst++ = ((pixel >> 3) & 0xFC); \
                            *dst++ = ((pixel & 0x1F) << 3););                                            \
            }

        if (source->format == RG_PIXEL_565_LE)
        {
            COPY_PIXELS(((uint16_t *)src)[src_x]);
        }
        else if (source->format == RG_PIXEL_565_BE)
        {
            COPY_PIXELS(((uint16_t *)src)[src_x]; pixel = (pixel << 8) | (pixel >> 8));
        }
        else if (source->format == RG_PIXEL_PAL565_LE)
        {
            COPY_PIXELS(source->palette[src[src_x]]);
        }
        else if (source->format == RG_PIXEL_PAL565_BE)
        {
            COPY_PIXELS(source->palette[src[src_x]]; pixel = (pixel << 8) | (pixel >> 8));
        }
        else if (source->format == RG_PIXEL_888)
        {
            COPY_PIXELS(({
                const uint8_t *pix = &src[src_x * 3];
                (((pix[0] << 8) & 0xF800) | ((pix[1] << 3) & 0x7E0) | (((pix[2] >> 3) & 0x1F)));
            }));
        }
        else
        {
            RG_LOGE("Invalid source format?");
            return false;
        }
    }

    return true;
}

rg_surface_t *rg_surface_convert(const rg_surface_t *source, int new_width, int new_height, int new_format)
{
    CHECK_SURFACE(source, NULL);

    // TODO: Negative dimensions should flip the image
    if (new_width <= 0 && new_height <= 0)
        new_width = source->width, new_height = source->height;
    else if (new_width <= 0)
        new_width = source->width * ((float)new_height / source->height);
    else if (new_height <= 0)
        new_height = source->height * ((float)new_width / source->width);

    if (new_format == 0)
        new_format = source->format;

    rg_surface_t *dest = rg_surface_create(new_width, new_height, new_format, 0);
    if (!dest)
    {
        RG_LOGW("Out of memory!\n");
        return NULL;
    }
    rg_surface_copy(source, NULL, dest, NULL, true);
    return dest;
}

rg_surface_t *rg_surface_load_image(const uint8_t *data, size_t data_len, uint32_t flags)
{
    RG_ASSERT(data && data_len >= 16, "bad param");

    if (memcmp(data, "\x89PNG", 4) == 0)
    {
        unsigned error, width, height;
        uint8_t *image = NULL;

        error = lodepng_decode24(&image, &width, &height, data, data_len);
        if (error)
        {
            RG_LOGE("PNG decoding failed: %d\n", error);
            return NULL;
        }

        rg_surface_t *img = rg_surface_create(width, height, RG_PIXEL_565_LE, 0);
        if (!img)
        {
            RG_LOGE("Out of memory");
            free(image);
            return NULL;
        }

        const uint8_t *src = image;
        uint16_t *dest = img->data;
        size_t pixel_count = width * height;
        while (pixel_count--)
        {
            *dest++ = (((src[0] << 8) & 0xF800) | ((src[1] << 3) & 0x7E0) | (((src[2] >> 3) & 0x1F)));
            src += 3;
        }
        free(image);
        return img;
    }

    // RAW565 (uint16 width, uint16 height, uint16 data[])
    int width = ((uint16_t *)data)[0];
    int height = ((uint16_t *)data)[1];

    if (data_len == width * height * 2 + 4)
    {
        rg_surface_t *img = rg_surface_create(width, height, RG_PIXEL_565_LE, 0);
        if (!img)
        {
            RG_LOGE("Out of memory");
            return NULL;
        }
        memcpy(img->data, data + 4, width * height * 2);
        return img;
    }

    RG_LOGE("Image format not recognized!");
    return NULL;
}

rg_surface_t *rg_surface_load_image_file(const char *filename, uint32_t flags)
{
    RG_ASSERT(filename, "bad param");

    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        RG_LOGE("Unable to open image file '%s'!\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);

    size_t data_len = ftell(fp);
    uint8_t *data = malloc(data_len);
    if (!data)
    {
        RG_LOGE("Memory allocation failed (%d bytes)!\n", (int)data_len);
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    fread(data, data_len, 1, fp);
    fclose(fp);

    rg_surface_t *img = rg_surface_load_image(data, data_len, flags);
    free(data);

    return img;
}

bool rg_surface_save_image_file(const rg_surface_t *source, const char *filename, int width, int height)
{
    CHECK_SURFACE(source, false);

    rg_surface_t *temp = NULL;
    int error = -1;

    if (width <= 0 && height <= 0)
        width = source->width, height = source->height;
    else if (width <= 0)
        width = source->width * ((float)height / source->height);
    else if (height <= 0)
        height = source->height * ((float)width / source->width);

    if (source->format != RG_PIXEL_888 || source->width != width || source->height != height)
    {
        temp = rg_surface_convert(source, width, height, RG_PIXEL_888);
        if (!temp)
            return false;
        source = temp;
    }

    error = lodepng_encode24_file(filename, source->data, width, height);
    rg_surface_free(temp);

    if (error == 0)
        return true;

    RG_LOGE("PNG encoding failed: %d\n", error);
    return false;
}
