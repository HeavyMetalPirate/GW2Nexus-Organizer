#ifndef NOTIFICATION_SERVICE_H
#define NOTIFICATION_SERVICE_H

#include "../Globals.h"

namespace toast {
	struct ToastData {
		/// <summary>
		/// Toast Id. To avoid collisions, prefix with something addon specific
		/// </summary>
		std::string toastId;
		/// <summary>
		/// The title displayed in the toast
		/// </summary>
		std::string title;
		/// <summary>
		/// The text summary displayed in the toast
		/// </summary>
		std::string text;
		/// <summary>
		/// Reference to a texture to be used for the toast
		/// </summary>
		Texture* texture;
		/// <summary>
		/// In case this is set, clicking the toast will copy the link to your clipboard
		/// </summary>
		std::string chatLink;
	};
}


class NotificationService {
public:
	void preRender();
	void render();
	void postRender();

	void unload();

	void queueToast(toast::ToastData* item);

private:
	std::vector<toast::ToastData*> toastQueue = std::vector<toast::ToastData*>();
	std::map<std::string, float> toastOpacity = std::map<std::string, float>();

	void animateToast(std::string id);
};

#endif