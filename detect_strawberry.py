from __future__ import division
import cv2
#to show the image
from matplotlib import pyplot as plt
import numpy as np
from math import cos, sin

green = (0, 255, 0)
red = (255, 0, 0)

def show(image):
    # Figure size in inches
    plt.figure(figsize=(10, 10))

    # Show image, with nearest neighbour interpolation
    plt.imshow(image, interpolation='nearest')

def overlay_mask(mask, image):
	#make the mask rgb
    rgb_mask = cv2.cvtColor(mask, cv2.COLOR_GRAY2RGB)
    #calculates the weightes sum of two arrays. in our case image arrays
    #input, how much to weight each.
    #optional depth value set to 0 no need
    img = cv2.addWeighted(rgb_mask, 0.5, image, 0.5, 0)
    return img

def find_biggest_contour(image):
    # Copy
    image = image.copy()
    #input, gives all the contours, contour approximation compresses horizontal,
    #vertical, and diagonal segments and leaves only their end points. For example,
    #an up-right rectangular contour is encoded with 4 points.
    #Optional output vector, containing information about the image topology.
    #It has as many elements as the number of contours.
    #we dont need it
    contours, hierarchy = cv2.findContours(image, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)

    # Isolate largest contour
    contour_sizes = [(cv2.contourArea(contour), contour) for contour in contours]
    biggest_contour = max(contour_sizes, key=lambda x: x[0])[1]
    # img1= cv2.imread('C:/Users/Lenovo/Desktop/imagename.jpg')
    # img2=img1.copy()
    # for ppp in contours:
    #     re=cv2.drawContours(img2,ppp,-1,(0,0,255),2)
    #     cv2.imshow("ppp1",re)
    #     cv2.waitKey(0)
    mask = np.zeros(image.shape, np.uint8)
    cv2.drawContours(mask, [biggest_contour], -1, 255, -1)
    return biggest_contour, mask

def circle_contour_red(image, contour):
    # Bounding ellipse
    image_with_ellipse = image.copy()
    #easy function
    ellipse = cv2.fitEllipse(contour)
    print(ellipse)
    #add it
    cv2.ellipse(image_with_ellipse, ellipse, red, 2,cv2.LINE_AA)
    return image_with_ellipse

def rectangle_coutour_red(image,contour):
    image_with_rectangle = image.copy()
    # easy function
    x, y, w, h = cv2.boundingRect(contour)
    cv2.rectangle(image_with_rectangle, (x, y), (x + w, y + h), (0, 0, 255), 2, 8, 0)
    # add it
    return image_with_rectangle,x,y,w,h
def circle_contour_green(image, contour):
    # Bounding ellipse
    image_with_ellipse = image.copy()
    #easy function
    ellipse = cv2.fitEllipse(contour)
    #add it
    cv2.ellipse(image_with_ellipse, ellipse, green, 2,cv2.LINE_AA)
    return image_with_ellipse

def find_strawberry_red(image):
    #RGB stands for Red Green Blue. Most often, an RGB color is stored
    #in a structure or unsigned integer with Blue occupying the least
    #significant “area” (a byte in 32-bit and 24-bit formats), Green the
    #second least, and Red the third least. BGR is the same, except the
    #order of areas is reversed. Red occupies the least significant area,
    # Green the second (still), and Blue the third.
    # we'll be manipulating pixels directly
    #most compatible for the transofrmations we're about to do
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    # Make a consistent size
    #get largest dimension
    max_dimension = max(image.shape)
    # print(image.shape)
    #The maximum window size is 700 by 660 pixels. make it fit in that
    scale = 700/max_dimension
    #resize it. same width and hieght none since output is 'image'.
    image = cv2.resize(image, None, fx=scale, fy=scale)


    #we want to eliminate noise from our image. clean. smooth colors without
    #dots
    # Blurs an image using a Gaussian filter. input, kernel size, how much to filter, empty)
    image_blur = cv2.GaussianBlur(image, (7, 7), 0)
    #t unlike RGB, HSV separates luma, or the image intensity, from
    # chroma or the color information.
    #just want to focus on color, segmentation
    image_blur_hsv = cv2.cvtColor(image_blur, cv2.COLOR_RGB2HSV)

    # Filter by colour
    # 0-10 hue
    #minimum red amount, max red amount
    min_red = np.array([0, 100, 80])
    max_red = np.array([10, 256, 256])
    #layer
    mask1 = cv2.inRange(image_blur_hsv, min_red, max_red)

    #birghtness of a color is hue
    # 170-180 hue
    min_red2 = np.array([170, 100, 80])
    max_red2 = np.array([180, 255, 255])
    mask2 = cv2.inRange(image_blur_hsv, min_red2, max_red2)


    lower_green = np.array([35, 43, 46])
    upper_green = np.array([77, 256, 256])
    maskGreen = cv2.inRange(image_blur_hsv, lower_green, upper_green)

    #looking for what is in both ranges
    # Combine masks
    mask = mask1 + mask2

    # Clean up
    #we want to circle our strawberry so we'll circle it with an ellipse
    #with a shape of 15x15
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (15, 15))
    #morph the image. closing operation Dilation followed by Erosion.
    #It is useful in closing small holes inside the foreground objects,
    #or small black points on the object.
    mask_closed = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    #erosion followed by dilation. It is useful in removing noise
    mask_clean = cv2.morphologyEx(mask_closed, cv2.MORPH_OPEN, kernel)

    # Find biggest strawberry
    #get back list of segmented strawberries and an outline for the biggest one
    big_strawberry_contour, mask_strawberries = find_biggest_contour(mask_clean)

    # Overlay cleaned mask on image
    # overlay mask on image, strawberry now segmented
    overlay = overlay_mask(mask_clean, image)

    # Circle biggest strawberry
    #circle the biggest one
    # circled = circle_contour_red(overlay, big_strawberry_contour)
    circled,x,y,w,h = rectangle_coutour_red(overlay, big_strawberry_contour)
    show(circled)

    #we're done, convert back to original color scheme
    bgr = cv2.cvtColor(circled, cv2.COLOR_RGB2BGR)

    return bgr


def find_strawberry_green(image):
    # RGB stands for Red Green Blue. Most often, an RGB color is stored
    # in a structure or unsigned integer with Blue occupying the least
    # significant “area” (a byte in 32-bit and 24-bit formats), Green the
    # second least, and Red the third least. BGR is the same, except the
    # order of areas is reversed. Red occupies the least significant area,
    # Green the second (still), and Blue the third.
    # we'll be manipulating pixels directly
    # most compatible for the transofrmations we're about to do
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    # Make a consistent size
    # get largest dimension
    max_dimension = max(image.shape)
    # print(image.shape)
    # The maximum window size is 700 by 660 pixels. make it fit in that
    scale = 700 / max_dimension
    # resize it. same width and hieght none since output is 'image'.
    image = cv2.resize(image, None, fx=scale, fy=scale)

    # we want to eliminate noise from our image. clean. smooth colors without
    # dots
    # Blurs an image using a Gaussian filter. input, kernel size, how much to filter, empty)
    image_blur = cv2.GaussianBlur(image, (7, 7), 0)
    # t unlike RGB, HSV separates luma, or the image intensity, from
    # chroma or the color information.
    # just want to focus on color, segmentation
    image_blur_hsv = cv2.cvtColor(image_blur, cv2.COLOR_RGB2HSV)

    lower_green = np.array([35, 43, 46])
    upper_green = np.array([77, 256, 256])
    maskGreen = cv2.inRange(image_blur_hsv, lower_green, upper_green)

    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (15, 15))

    mask_closed1 = cv2.morphologyEx(maskGreen, cv2.MORPH_CLOSE, kernel)
    mask_clean1 = cv2.morphologyEx(mask_closed1, cv2.MORPH_OPEN, kernel)
    big_strawberry_contour1, mask_strawberries1 = find_biggest_contour(mask_clean1)
    overlay1 = overlay_mask(mask_clean1, image)
    circled1 = circle_contour_green(overlay1, big_strawberry_contour1)
    show(circled1)
    bgr1 = cv2.cvtColor(circled1, cv2.COLOR_RGB2BGR)

    return bgr1

#read the image
image = cv2.imread('test.jpg')

#detect it
result_red = find_strawberry_red(image)
# result_green = find_strawberry_green(image)
#write the new image
cv2.imwrite('result_R.jpg', result_red)
# cv2.imwrite('result_G.jpg', result_green)
# img = cv2.addWeighted(result_red, 0.5, result_green, 0.5, 0)
cv2.imshow("PPP", result_red)

cv2.waitKey(0)