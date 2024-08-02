#ifndef SHARED_SERVICES_H
#define SHARED_SERVICES_H

#include "OrganizerRepository.h"
#include "ApiTokenService.h"
#include "NotificationService.h"

extern OrganizerRepository* organizerRepo;
extern ApiTokenService apiTokenService;
extern NotificationService notificationService;

#endif