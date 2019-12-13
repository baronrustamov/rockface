/****************************************************************************
*
*    Copyright (c) 2017 - 2019 by Rockchip Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Rockchip Corporation. This is proprietary information owned by
*    Rockchip Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Rockchip Corporation.
*
*****************************************************************************/
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "rockface.h"
#include "face_db.h"
#include "rockface_recognition.h"

static int get_face_library(rockface_handle_t handle, const char *db_path) {

    sqlite3 *db = NULL;

    int _face_num = 0;
    face_data *_face_array = NULL;
    rockface_ret_t rockface_ret = ROCKFACE_RET_SUCCESS;
    int ret = 0;

    ret = open_db(db_path, &db);

    ret = get_face_count(db, &_face_num);
    if (ret < 0) {
        printf("error get_face_count %d\n", ret);
        goto error;
    }

    _face_array = (face_data *)malloc(_face_num*sizeof(face_data));
    ret = get_all_face(db, _face_array, _face_num);
    if (ret < 0) {
        printf("error get_face_count %d\n", ret);
        goto error;
    }

    rockface_ret = rockface_face_library_init(handle, _face_array, _face_num, sizeof(face_data), 0);
    if (rockface_ret != ROCKFACE_RET_SUCCESS) {
        printf("error rockface_face_library_init %d\n", rockface_ret);
        ret = -1;
        goto error;
    }

error:
    close_db(db);
    if (_face_array != NULL) {
        free(_face_array);
    }

    return ret;
}

int main(int argc, char** argv) {

    rockface_ret_t ret;
    struct timeval tv;

    if( argc != 4 ){
        printf("\nUsage: recognition_1vn <licence file> <path_to_image> <path_to_database_path>\n");
        return -1;
    }

    const char *licence_path = argv[1];

    // read image
    const char *img_path = argv[2];
    const char *db_path = argv[3];

    /*************** Creat Handle ***************/
    rockface_handle_t face_handle = rockface_create_handle();
    rockface_feature_t face_feature;
    rockface_image_t in_img;
    rockface_search_result_t search_result;
    rockface_ret_t rockface_ret;

    ret = rockface_set_licence(face_handle, licence_path);
    if (ret < 0) {
        printf("Error: authorization error %d!", ret);
        return ret;
    }

    ret = rockface_init_detector(face_handle);
    ret = rockface_init_recognizer(face_handle);

    //
    int db_ret = get_face_library(face_handle, db_path);
    if (db_ret != 0) {
        printf("get face library error\n");
        goto error;
    }

    rockface_image_read(img_path, &in_img, 1);
    ret = run_face_recognize(face_handle, &in_img, &face_feature);
    if (ret != ROCKFACE_RET_SUCCESS) {
        printf("get face feature error");
        goto error;
    }

    gettimeofday(&tv, NULL);
    printf("%ld before search\n", (tv.tv_sec * 1000 + tv.tv_usec/1000));

    rockface_ret = rockface_feature_search(face_handle, &face_feature, 0.7, &search_result);
    
    gettimeofday(&tv, NULL);
    printf("%ld after search\n", (tv.tv_sec * 1000 + tv.tv_usec/1000));

    if (rockface_ret != ROCKFACE_RET_SUCCESS) {
        printf("error rockface_feature_search %d\n", rockface_ret);
    }
    printf("search result: name=%s dist=%f\n", ((face_data *)search_result.feature)->name, search_result.similarity);

error:
    rockface_image_release(&in_img);
    rockface_face_library_release(face_handle);
    rockface_release_handle(face_handle);

    return 0;
}
