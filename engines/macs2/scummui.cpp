/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "macs2/scummui.h"
#include "macs2/macs2.h"
#include "macs2/view1.h"
#include "macs2/gameobjects.h"

namespace Macs2 {

const ScummUI::VerbDef ScummUI::kVerbs[4] = {
	{"Walk", Script::MouseMode::Walk},
	{"Look", Script::MouseMode::Look},
	{"Use", Script::MouseMode::Use},
	{"Talk", Script::MouseMode::Talk}
};

ScummUI::ScummUI(View1 *view)
	: _view(view), _hoveredVerb(-1), _hoveredItemIndex(-1), _inventoryScrollOffset(0) {
}

bool ScummUI::isPointInUI(const Common::Point &pos) const {
	return pos.y >= kUITop;
}

void ScummUI::draw(Graphics::ManagedSurface &s) {
	// Dark background for entire UI area (color 0 = always black)
	s.fillRect(Common::Rect(0, kUITop, 320, kUITop + kUIHeight), 0);
	drawSentenceLine(s);
	drawVerbBar(s);
	drawInventoryStrip(s);
}

void ScummUI::drawSentenceLine(Graphics::ManagedSurface &s) {
	// Build sentence: "Verb" or "Verb object"
	Common::String sentence;
	Script::MouseMode mode = g_engine->_scriptExecutor->_mouseMode;

	for (int i = 0; i < 4; i++) {
		if (kVerbs[i].mode == mode) {
			sentence = kVerbs[i].label;
			break;
		}
	}
	if (mode == Script::MouseMode::UseInventory && _view->_activeInventoryItem) {
		sentence = "Use item";
	}
	if (!_sentenceObject.empty()) {
		sentence += " " + _sentenceObject;
	}

	if (!sentence.empty()) {
		_view->renderStringTo(4, kSentenceY + 2, sentence, s);
	}
}

void ScummUI::drawVerbBar(Graphics::ManagedSurface &s) {
	Script::MouseMode activeMode = g_engine->_scriptExecutor->_mouseMode;

	for (int i = 0; i < 4; i++) {
		Common::Rect r = getVerbRect(i);
		bool isActive = (kVerbs[i].mode == activeMode);
		bool isHovered = (i == _hoveredVerb);

		// Active/hovered verb: draw a border to indicate state
		// Use only color 0 (black bg) - verbs distinguished by border
		if (isActive) {
			// Double border for active verb
			s.frameRect(r, 0);
			Common::Rect inner(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1);
			s.frameRect(inner, 0);
		} else if (isHovered) {
			s.frameRect(r, 0);
		}

		// Draw label centered - glyphs have their own color
		int textX = r.left + (r.width() - (int)strlen(kVerbs[i].label) * 6) / 2;
		int textY = r.top + (r.height() - 8) / 2;
		_view->renderStringTo(textX, textY, kVerbs[i].label, s);

		// Mark active verb with an underline made of glyph pixels
		if (isActive) {
			_view->renderStringTo(textX, textY + 10, "----", s);
		}
	}
}

void ScummUI::drawInventoryStrip(Graphics::ManagedSurface &s) {
	// Draw scroll arrows using text glyphs
	Common::Rect leftArrow = getInvScrollLeftRect();
	Common::Rect rightArrow = getInvScrollRightRect();
	_view->renderStringTo(leftArrow.left + 4, leftArrow.top + 7, "<", s);
	_view->renderStringTo(rightArrow.left + 4, rightArrow.top + 7, ">", s);

	// Draw inventory items (always protagonist's inventory)
	const Common::Array<GameObject *> items = getProtagonistItems();
	int maxVisible = kInvCols * kInvRows;
	for (int i = 0; i < maxVisible; i++) {
		int itemIdx = _inventoryScrollOffset + i;
		if (itemIdx >= (int)items.size())
			break;

		Common::Rect r = getInvItemRect(i);
		bool isHovered = (i == _hoveredItemIndex);
		bool isActive = (_view->_activeInventoryItem == items[itemIdx]);

		// Draw item icon
		AnimFrame *icon = _view->getInventoryIcon(items[itemIdx]);
		if (icon && icon->_data) {
			int iconX = r.left + (r.width() - icon->_width) / 2;
			int iconY = r.top + (r.height() - icon->_height) / 2;
			_view->drawSprite(iconX, iconY, icon->_width, icon->_height, icon->_data, s, false);
		}

		// Active/hovered indicator using text markers (palette-independent)
		if (isActive) {
			_view->renderStringTo(r.left + 1, r.top, "*", s);
		} else if (isHovered) {
			_view->renderStringTo(r.left + 1, r.top, ".", s);
		}
	}
}

Common::Array<GameObject *> ScummUI::getProtagonistItems() const {
	Common::Array<GameObject *> items;
	GameObject *protagonist = GameObjects::instance().getProtagonistObject();
	if (!protagonist)
		return items;
	const uint16 invScene = protagonist->_index + 0x400;
	for (GameObject *obj : GameObjects::instance()._objects) {
		if (obj && obj->_sceneIndex == invScene)
			items.push_back(obj);
	}
	return items;
}

bool ScummUI::handleClick(const Common::Point &pos) {
	// Check verb buttons
	for (int i = 0; i < 4; i++) {
		if (getVerbRect(i).contains(pos)) {
			g_engine->setCursorMode(kVerbs[i].mode);
			_view->_activeInventoryItem = nullptr;
			_view->updateCursor();
			return true;
		}
	}

	// Check scroll arrows
	if (getInvScrollLeftRect().contains(pos)) {
		if (_inventoryScrollOffset > 0)
			_inventoryScrollOffset -= kInvCols * kInvRows;
		return true;
	}
	if (getInvScrollRightRect().contains(pos)) {
		int maxItems = (int)getProtagonistItems().size();
		int maxVisible = kInvCols * kInvRows;
		if (_inventoryScrollOffset + maxVisible < maxItems)
			_inventoryScrollOffset += maxVisible;
		return true;
	}

	// Check inventory items (always protagonist's)
	Common::Array<GameObject *> items = getProtagonistItems();
	int maxVisible = kInvCols * kInvRows;
	for (int i = 0; i < maxVisible; i++) {
		int itemIdx = _inventoryScrollOffset + i;
		if (itemIdx >= (int)items.size())
			break;

		if (getInvItemRect(i).contains(pos)) {
			GameObject *item = items[itemIdx];
			Script::MouseMode mode = g_engine->_scriptExecutor->_mouseMode;

			if (mode == Script::MouseMode::Look) {
				// Look at item - same flow as world object interaction
				g_engine->_scriptExecutor->_interactedObjectID = 0x400 + item->_index;
				g_engine->_scriptExecutor->_interactedOtherObjectID = 0;
				g_engine->_scriptExecutor->setCurrentSceneScriptAt(0);
				g_engine->runScriptExecutor(false);
				g_engine->_scriptExecutor->_interactedObjectID = 0;
			} else if (mode == Script::MouseMode::Use) {
				// Select item for use-with
				_view->_activeInventoryItem = item;
				g_engine->_scriptExecutor->_interactedOtherObjectID = 0x400 + item->_index;
				AnimFrame *icon = _view->getInventoryIcon(item);
				if (icon) {
					int cursorSlot = (int)Script::MouseMode::UseInventory - 1;
					uint32 pixelSize = icon->_width * icon->_height;
					delete[] g_engine->_imageResources[cursorSlot]._data;
					g_engine->_imageResources[cursorSlot]._data = new byte[pixelSize];
					memcpy(g_engine->_imageResources[cursorSlot]._data, icon->_data, pixelSize);
					g_engine->_imageResources[cursorSlot]._width = icon->_width;
					g_engine->_imageResources[cursorSlot]._height = icon->_height;
				}
				g_engine->setCursorMode(Script::MouseMode::UseInventory);
				_view->updateCursor();
			} else if (mode == Script::MouseMode::UseInventory && _view->_activeInventoryItem) {
				// Combine items
				g_engine->_scriptExecutor->_interactedObjectID = 0x400 + _view->_activeInventoryItem->_index;
				g_engine->_scriptExecutor->_interactedOtherObjectID = 0x400 + item->_index;
				g_engine->runScriptExecutor(false);
			}
			return true;
		}
	}

	return true;
}

void ScummUI::handleMouseMove(const Common::Point &pos) {
	_hoveredVerb = -1;
	_hoveredItemIndex = -1;

	for (int i = 0; i < 4; i++) {
		if (getVerbRect(i).contains(pos)) {
			_hoveredVerb = i;
			return;
		}
	}

	Common::Array<GameObject *> items = getProtagonistItems();
	int maxVisible = kInvCols * kInvRows;
	for (int i = 0; i < maxVisible; i++) {
		int itemIdx = _inventoryScrollOffset + i;
		if (itemIdx >= (int)items.size())
			break;
		if (getInvItemRect(i).contains(pos)) {
			_hoveredItemIndex = i;
			return;
		}
	}
}

void ScummUI::updateSentenceLine(const Common::String &objectName) {
	_sentenceObject = objectName;
}

void ScummUI::clearSentenceObject() {
	_sentenceObject.clear();
}

Common::Rect ScummUI::getVerbRect(int index) const {
	int col = index % kVerbCols;
	int row = index / kVerbCols;
	int x = col * kVerbW;
	int y = kVerbY + row * kVerbH;
	return Common::Rect(x, y, x + kVerbW, y + kVerbH);
}

Common::Rect ScummUI::getInvItemRect(int index) const {
	int col = index % kInvCols;
	int row = index / kInvCols;
	int x = kInvX + kInvArrowW + col * kInvItemW;
	int y = kVerbY + row * kInvItemH;
	return Common::Rect(x, y, x + kInvItemW, y + kInvItemH);
}

Common::Rect ScummUI::getInvScrollLeftRect() const {
	return Common::Rect(kInvX, kVerbY, kInvX + kInvArrowW, kVerbY + kVerbH * kVerbRows);
}

Common::Rect ScummUI::getInvScrollRightRect() const {
	int x = kInvX + kInvArrowW + kInvCols * kInvItemW;
	return Common::Rect(x, kVerbY, x + kInvArrowW, kVerbY + kVerbH * kVerbRows);
}

} // namespace Macs2
