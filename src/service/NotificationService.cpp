#include "NotificationService.h"
#include <thread>

void NotificationService::preRender() {
	// TODO impl
}
void NotificationService::render() {
	if (toastQueue.empty()) return;
	// TODO figure out how the thread should remove the toast from the queue once rendering is done
	// we have an opacity map now, render by opacity and the render thread updates all the opacities and will
	// drop all toasts that have reach opacity 0 again

	// Save current style details
	ImGuiStyle& style = ImGui::GetStyle();
	float oldBorderSize = style.WindowBorderSize;
	ImVec4 oldBorderColor = style.Colors[ImGuiCol_Border];
	float currentAlpha = style.Alpha;

	// Window Border
	style.WindowBorderSize = 2.0f;  // Adjust border size as needed
	style.Colors[ImGuiCol_Border] = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);

	// Start position etc.
	const float windowHeight = settings.notifications.height > 0.0f ? settings.notifications.height : 50.0f;
	const float windowWidth = settings.notifications.width > 0.0f ? settings.notifications.width : 300.0f;
	ImVec2 windowPos = ImVec2(settings.notifications.x, settings.notifications.y);

	for (int i = 0; i < toastQueue.size(); i++) {
		auto data = toastQueue.at(i);
		if (data == nullptr) continue; // skip if no data to display
		// set opacity based on map data
		float opacity = toastOpacity[data->toastId];
#ifndef NDEBUG
		APIDefs->Log(ELogLevel_ALL, ADDON_NAME, ("Opacity for Toast " + data->toastId + ": " + std::to_string(opacity)).c_str());
#endif
		if (opacity == 0.0f) continue; // skip if no opacity set (yet) - cleanup should be done by animation thread once it knows it's done

		ImGui::GetStyle().Alpha = opacity;
		// set position
		ImGui::SetNextWindowPos(windowPos);
		ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
		if (ImGui::Begin(data->toastId.c_str(), nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | 
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar)) {
			// TODO more extensive styling in case we have stuff like images or links?
			ImGui::TextWrapped(data->title.c_str());
			ImGui::TextWrapped(data->text.c_str());
			ImGui::End();
		}
		// Move the position for the next window down by the height of the current window
		if (settings.notifications.direction == 0) {
			windowPos.y += windowHeight;
		}
		else if (settings.notifications.direction == 1) {
			windowPos.y -= 2 * windowHeight;
		}
		else {
			windowPos.y += windowHeight; // default fallback
		}
	}

	// Restore previous styles
	style.Alpha = currentAlpha;
	style.WindowBorderSize = oldBorderSize;
	style.Colors[ImGuiCol_Border] = oldBorderColor;
}
void NotificationService::postRender() {
	// TODO impl
}

void NotificationService::unload() {
	unloading = true;
}

void NotificationService::queueToast(toast::ToastData* data) {
	if (!settings.notifications.enabled) return; // disabled

#ifndef NDEBUG
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Queued Toast: " + data->title).c_str());
#endif

	toastQueue.push_back(data);
	toastOpacity.emplace(data->toastId, 0.0f);
	std::thread animate = std::thread(&NotificationService::animateToast, this, data->toastId);
	animate.detach(); // YOLO
}

void NotificationService::animateToast(std::string toastId) {
#ifndef NDEBUG
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Toast: " + toastId).c_str());
#endif

	try {
		// fade in
		while (true)
		{
			if (unloading) {
				toastOpacity[toastId] = 0.0f;
				return;
			}
			toastOpacity[toastId] += 0.10f;
			if (toastOpacity[toastId] > 1.0f) {
				toastOpacity[toastId] = 1.0f;
				break;
			}

			Sleep(35);
		}
		int duration = settings.notifications.duration > 0 ? settings.notifications.duration : 4000;
		for (int i = 0; i < duration; i++) {
			if (unloading) {
				toastOpacity[toastId] = 0.0f;
				return;
			}
			Sleep(1); // sleep first so the text stays a little
		}
		// fade out
		while (true)
		{
			if (unloading) {
				toastOpacity[toastId] = 0.0f;
				return;
			}

			toastOpacity[toastId] -= 0.10f;

			if (toastOpacity[toastId] < 0.0f) {
				toastOpacity[toastId] = 0.0f;
				break;
			}

			Sleep(35);
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Exception in animation thread.");
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, e.what());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception thread.");
	}
	toastQueue.erase(
		std::remove_if(
			toastQueue.begin(), toastQueue.end(),
			[&toastId](const toast::ToastData* element) {
				return element->toastId == toastId;
			}
		),
		toastQueue.end()
	);
}