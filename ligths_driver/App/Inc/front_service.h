/**
  * @file front_service.h
  * @brief Service entry points for the front (LF/RF) light boards.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef FRONT_SERVICE_H
#define FRONT_SERVICE_H

/**
  * @brief Service routine for the left-front light board.
  */
void LF_Service(void);

/**
  * @brief Service routine for the right-front light board.
  */
void RF_Service(void);

#endif // FRONT_SERVICE_H
