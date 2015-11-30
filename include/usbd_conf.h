/**
  ******************************************************************************
  * @file    USB_Device/CDC_Standalone/Inc/usbd_conf.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    13-March-2014
  * @brief   General low level driver configuration
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CONF_H
#define __USBD_CONF_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Common Config */
#define USBD_MAX_NUM_INTERFACES               1
#define USBD_MAX_NUM_CONFIGURATION            1
#define MAX_2(A, B) ((A)>(B)?(A):(B))
#define MAX_3(A, B, C) ((A)>((B)>(C)?(B):(C))?(A):((B)>(C)?(B):(C)))
#define MAX_4(A, B, C, D) (((A)>(B)?(A):(B))>((C)>(D)?(C):(D))?((A)>(B)?(A):(B)):((C)>(D)?(C):(D)))
#define MAX_5(A, B, C, D, E) (((A)>(B)?(A):(B))>((C)>((D)>(E)?(D):(E))?(C):((D)>(E)?(D):(E)))?((A)>(B)?(A):(B)):((C)>((D)>(E)?(D):(E))?(C):((D)>(E)?(D):(E))))
#define MAX_6(A, B, C, D, E, F) (((A)>((B)>(C)?(B):(C))?(A):((B)>(C)?(B):(C)))>((D)>((E)>(F)?(E):(F))?(D):((E)>(F)?(E):(F)))?((A)>((B)>(C)?(B):(C))?(A):((B)>(C)?(B):(C))):((D)>((E)>(F)?(E):(F))?(D):((E)>(F)?(E):(F))))
#define MAX_7(A, B, C, D, E, F, G) (((A)>((B)>(C)?(B):(C))?(A):((B)>(C)?(B):(C)))>(((D)>(E)?(D):(E))>((F)>(G)?(F):(G))?((D)>(E)?(D):(E)):((F)>(G)?(F):(G)))?((A)>((B)>(C)?(B):(C))?(A):((B)>(C)?(B):(C))):(((D)>(E)?(D):(E))>((F)>(G)?(F):(G))?((D)>(E)?(D):(E)):((F)>(G)?(F):(G))))
#define MAX_8(A, B, C, D, E, F, G, H) ((((A)>(B)?(A):(B))>((C)>(D)?(C):(D))?((A)>(B)?(A):(B)):((C)>(D)?(C):(D)))>(((E)>(F)?(E):(F))>((G)>(H)?(G):(H))?((E)>(F)?(E):(F)):((G)>(H)?(G):(H)))?(((A)>(B)?(A):(B))>((C)>(D)?(C):(D))?((A)>(B)?(A):(B)):((C)>(D)?(C):(D))):(((E)>(F)?(E):(F))>((G)>(H)?(G):(H))?((E)>(F)?(E):(F)):((G)>(H)?(G):(H))))
#define MAX_9(A, B, C, D, E, F, G, H, I) ((((A)>(B)?(A):(B))>((C)>(D)?(C):(D))?((A)>(B)?(A):(B)):((C)>(D)?(C):(D)))>(((E)>(F)?(E):(F))>((G)>((H)>(I)?(H):(I))?(G):((H)>(I)?(H):(I)))?((E)>(F)?(E):(F)):((G)>((H)>(I)?(H):(I))?(G):((H)>(I)?(H):(I))))?(((A)>(B)?(A):(B))>((C)>(D)?(C):(D))?((A)>(B)?(A):(B)):((C)>(D)?(C):(D))):(((E)>(F)?(E):(F))>((G)>((H)>(I)?(H):(I))?(G):((H)>(I)?(H):(I)))?((E)>(F)?(E):(F)):((G)>((H)>(I)?(H):(I))?(G):((H)>(I)?(H):(I)))))


#define USBD_MAX_STR_DESC_SIZ                 2+2*(MAX_9(sizeof (USBD_MANUFACTURER_STRING), sizeof (USBD_PRODUCT_HS_STRING), sizeof (USBD_SERIALNUMBER_HS_STRING), sizeof (USBD_PRODUCT_FS_STRING), sizeof (USBD_SERIALNUMBER_FS_STRING), sizeof (USBD_CONFIGURATION_HS_STRING), sizeof (USBD_INTERFACE_HS_STRING), sizeof (USBD_CONFIGURATION_FS_STRING), sizeof (USBD_INTERFACE_FS_STRING )))
#define USBD_SUPPORT_USER_STRING              0
#define USBD_SELF_POWERED                     1
#define USBD_DEBUG_LEVEL                      0
 
/* Exported macro ------------------------------------------------------------*/
/* Memory management macros */
#define USBD_malloc               malloc
#define USBD_free                 free
#define USBD_memset               memset
#define USBD_memcpy               memcpy
    
/* DEBUG macros */
#if (USBD_DEBUG_LEVEL > 0)
#define  USBD_UsrLog(...)   printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1)

#define  USBD_ErrLog(...)   printf("ERROR: ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define  USBD_DbgLog(...)   printf("DEBUG : ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_DbgLog(...)
#endif

/* Exported functions ------------------------------------------------------- */

#endif /* __USBD_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
