#include "otpch.h"

#include "player.h"
#include "monsters.h"
#include "items.h"
#include "game.h"

// NOTE! SQL query has to be executed
// ALTER TABLE `players` ADD `tasks` BLOB NULL DEFAULT NULL AFTER `conditions`;

//	[1] = {
//		className = "TROLLS",
//		monsters = { "Troll", "Island Troll", "Swamp Troll", "Frost Troll" },
//		bossName = "Big Boss Trolliver",
//		experience = 6000,
//		count = 200,
//		storageId = 5001,
//		bonusStorageId = 5001,
//		classOutfit = { type = 15 },
//		rewards = { 2148, 4, 2152, 4, 2160, 1 }
//	},

extern Monsters g_monsters;
extern Game g_game;

template <typename T>
bool getTaskValue(lua_State* L, const char* node, T& value, bool ignore = false) {
	lua_getfield(L, -1, node);
	if (!lua_isnumber(L, -1)) {
		if (!ignore) {
			std::cout << "[Warning - PlayerTasksData::load] Missing " << node << " property." << std::endl;
		}
		lua_pop(L, 1);
		return ignore;
	}
	value = static_cast<T>(lua_tonumber(L, -1));
	lua_pop(L, 1);
	return true;
};

bool PlayerTasksData::load()
{
	lua_State* L = luaL_newstate();
	if (!L) {
		throw std::runtime_error("Failed to allocate memory in PlayerTasksData");
	}

	if (luaL_dofile(L, "data/LUA/tasks.lua")) {
		std::cout << "[Error - PlayerTasksData] " << lua_tostring(L, -1) << std::endl;
		lua_close(L);
		return false;
	}

	lua_getglobal(L, "MAXIMUM_TASKS_AT_ONCE");
	if (lua_isnumber(L, -1)) {
		m_maximumTasksAtOnce = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	lua_getglobal(L, "MAXIMUM_PREMIUM_TASKS_AT_ONCE");
	if (lua_isnumber(L, -1)) {
		m_maximumPremiumTasksAtOnce = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	lua_getglobal(L, "GLOBAL_TASK_BONUS_STORAGE_ID");
	if (lua_isnumber(L, -1)) {
		m_globalTaskBonusStorgeId = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	lua_getglobal(L, "GLOBAL_TASK_BONUS_GLOBAL_STORAGE_ID");
	if (lua_isnumber(L, -1)) {
		m_globalTaskBonusGlobalStorgeId = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);
	
	lua_getglobal(L, "TASK_COUNT_FOR_MOST_DAMAGE");
	if (lua_isboolean(L, -1)) {
		m_taskCountForMostDamage = lua_toboolean(L, -1);
	}
	lua_pop(L, 1);

	lua_getglobal(L, "TASKS");
	for (uint16_t i = 1; ; i++) {
		lua_rawgeti(L, -1, i);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			break;
		}

		TaskData taskData;
		lua_getfield(L, -1, "bossName");
		if (lua_isstring(L, -1)) {
			taskData.bossName = lua_tostring(L, -1);
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "className");
		if (!lua_isstring(L, -1)) {
			std::cout << "[Warning - PlayerTasksData::load] Missing className property." << std::endl;
			return false;
		}
		taskData.className = lua_tostring(L, -1);
		lua_pop(L, 1);

		if (!getTaskValue(L, "cooldown", taskData.cooldown) ||
			!getTaskValue(L, "experience", taskData.experience) ||
			!getTaskValue(L, "maxRepeatQuantity", taskData.maxRepeatQuantity) ||
			!getTaskValue(L, "storageId", taskData.storageId) ||
			!getTaskValue(L, "bonusStorageId", taskData.bonusStorageId, true)) {
			return false;
		}

		lua_getfield(L, -1, "classOutfit");
		if (!lua_istable(L, -1)) {
			std::cout << "[Warning - PlayerTasksData::load] Missing classOutfit property." << std::endl;
			return false;
		}

		{
			lua_getfield(L, -1, "type");
taskData.classOutfit.lookType = lua_tonumber(L, -1);
lua_pop(L, 1);
lua_getfield(L, -1, "head");
taskData.classOutfit.lookHead = lua_tonumber(L, -1);
lua_pop(L, 1);
lua_getfield(L, -1, "body");
taskData.classOutfit.lookBody = lua_tonumber(L, -1);
lua_pop(L, 1);
lua_getfield(L, -1, "legs");
taskData.classOutfit.lookLegs = lua_tonumber(L, -1);
lua_pop(L, 1);
lua_getfield(L, -1, "feet");
taskData.classOutfit.lookFeet = lua_tonumber(L, -1);
lua_pop(L, 1);
lua_getfield(L, -1, "addons");
taskData.classOutfit.lookAddons = lua_tonumber(L, -1);
lua_pop(L, 1);		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "monsters");
		if (!lua_istable(L, -1)) {
			std::cout << "[Warning - PlayerTasksData::load] Missing 'monsters' property." << std::endl;
			return false;
		}

		{
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				lua_getfield(L, -1, "name");

				std::vector<std::pair<std::string, Outfit_t>> names;
				if (lua_isstring(L, -1)) {
					// name = "Demon"
					const std::string name = lua_tostring(L, -1);
					lua_pop(L, 1);

					MonsterType* mType = g_monsters.getMonsterType(name);
					if (!mType) {
						std::cout << "[Warning - PlayerTasksData::load] Invalid monster's name '" << name << "'." << std::endl;
						return false;
					}

					names.emplace_back(name, mType->outfit);
				} else if (lua_istable(L, -1)) {
					// name = {"Demon", "Juggernaut", "Hero"}
					lua_pushnil(L);
					while (lua_next(L, -2)) {
						const std::string name = lua_tostring(L, -1);

						MonsterType* mType = g_monsters.getMonsterType(name);
						if (!mType) {
							std::cout << "[Warning - PlayerTasksData::load] Invalid monster's name '" << name << "'." << std::endl;
							return false;
						}

						names.emplace_back(name, mType->outfit);
						lua_pop(L, 1);
					}

					lua_pop(L, 1);
				} else {
					std::cout << "[Warning - PlayerTasksData::load] Missing monster 'name' property." << std::endl;
					return false;
				}

				lua_getfield(L, -1, "count");
				if (!lua_isstring(L, -1)) {
					std::cout << "[Warning - PlayerTasksData::load] Missing monster 'count' property." << std::endl;
					return false;
				}

				TaskProgress taskProgress;
				taskProgress.count = lua_tonumber(L, -1);
				taskProgress.names = names;
				lua_pop(L, 1);

				taskData.monsters.push_back(taskProgress);
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "rewards");
		if (!lua_istable(L, -1)) {
			std::cout << "[Warning - PlayerTasksData::load] Missing rewards property." << std::endl;
			return false;
		}

		{
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				lua_getfield(L, -1, "itemId");
				if (!lua_isnumber(L, -1)) {
					std::cout << "[Warning - PlayerTasksData::load] Missing rewards 'itemId' property." << std::endl;
					return false;
				}

				uint16_t itemId = lua_tonumber(L, -1);
				lua_pop(L, 1);

				lua_getfield(L, -1, "count");
				if (!lua_isnumber(L, -1)) {
					std::cout << "[Warning - PlayerTasksData::load] Missing rewards 'count' property." << std::endl;
					return false;
				}

				const ItemType& it = Item::items[itemId];
				if (it.id == 0) {
					std::cout << "[Warning - PlayerTasksData::load] Invalid item ID '" << itemId << "'." << std::endl;
					return false;
				}

				taskData.rewards.push_back({ itemId, it.clientId, lua_tonumber(L, -1) });
				lua_pop(L, 2);
			}
		}

		lua_pop(L, 1);

		lua_getfield(L, -1, "attributes");
		if (lua_istable(L, -1)) {
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				lua_getfield(L, -1, "name");
				if (!lua_isstring(L, -1)) {
					std::cout << "[Warning - PlayerTasksData::load] Missing attributes 'name' property." << std::endl;
					return false;
				}

				std::string name = lua_tostring(L, -1);
				lua_pop(L, 1);

				lua_getfield(L, -1, "value");
				if (!lua_isnumber(L, -1)) {
					std::cout << "[Warning - PlayerTasksData::load] Missing attributes 'value' property." << std::endl;
					return false;
				}

				taskData.attributes.push_back({ name, static_cast<double>(lua_tonumber(L, -1)) });
				lua_pop(L, 2);
			}
		}

		m_playerTasks.emplace_back(taskData.className, taskData);
		lua_pop(L, 2);
	}

	lua_close(L);
	return true;
}

bool PlayerTasksData::reload()
{
	m_playerTasks.clear();
	return load();
}

TaskData* PlayerTasksData::getTaskByName(const std::string& name)
{
	for (auto& [taskName, taskData] : m_playerTasks) {
		if (name == taskName) {
			return &taskData;
		}
	}

	std::cout << "[Warning - PlayerTasksData::getTaskByName] - Task name '" << name << "' does not exist.\n";
	return nullptr;
}

const char* PlayerTasksData::getReturnMessage(ReturnTaskMessages ret) const
{
	switch (ret) {
		case RET_TASK_ALREADY_TAKEN:
			return "Task is already taken.";
		case RET_TASK_IS_NOT_ACTIVE:
			return "You cannot cancel this task because it is not taken.";
		case RET_TASK_IS_COMPLETED:
			return "You cannot cancel this task because it is alredy completed.";
		case RET_TASK_IS_NOT_COMPLETED:
			return "You cannot claim rewards because this task is not completed yet.";
		case RET_TASK_LIMIT_HAS_BEEN_REACHED:
			return "You cannot select this task because the limit has been reached.";
		default:
			return "";
	}
}

uint16_t PlayerTasksData::getMaximumTaskAtOnce(Player* player)
{
	return player->isPremium() ? m_maximumPremiumTasksAtOnce : m_maximumTasksAtOnce;
}

void TaskData::updateTask(const Player* player, const std::string& name) {
	for (auto& taskProgress : this->monsters) {
		if (taskProgress.kills >= taskProgress.count) {
			continue;
		}

		if (taskProgress.canUse(name)) {
			const auto timeNow = (OTSYS_TIME() / 1000);
			if (g_game.getGlobalStorageValue(PlayerTasksData::getInstance()->m_globalTaskBonusGlobalStorgeId) >= timeNow) {
				taskProgress.kills++;
			}
			else {
				int32_t storageId = PlayerTasksData::getInstance()->m_globalTaskBonusStorgeId;
				if (storageId != 0) {
					int32_t value = 0;
					if (player->getStorageValue(storageId, value) && value >= timeNow) {
						taskProgress.kills++;
					}
				}
			}

			if (taskProgress.kills < taskProgress.count) {
				taskProgress.kills++;
			}
			break;
		}
	}
}

bool TaskData::isBonusTask(const Player* player) const
{
	if (bonusStorageId == 0) {
		return false;
	}

	int32_t value = 0;
	if (player->getStorageValue(bonusStorageId, value)) {
		return value == 1;
	}
	return false;
}

ReturnTaskMessages PlayerTasks::manageTask(Player* player, uint8_t id, const std::string& taskName)
{
	switch (id) {
		case 1: {
			// Claim rewards
			return completeTask(player, taskName);
		}
		case 2: {
			// Select task
			return selectTask(player, taskName);
		}
		case 3: {
			// Cancel task
			return cancelTask(taskName);
		}
		default:
			break;
	}

	return RET_TASK_NO_ERROR;
}

ReturnTaskMessages PlayerTasks::completeTask(Player* player, const std::string& taskName)
{
	if (!hasActiveTask(taskName)) {
		return RET_TASK_IS_NOT_ACTIVE;
	}

	TaskData* task = getTask(taskName);
	if (!task || !task->isCompleted()) {
		return RET_TASK_IS_NOT_COMPLETED;
	}

	Item* mailItem = nullptr;
	for (auto& [itemId, clientId, itemCount] : task->rewards) {
		uint16_t count = itemCount;
		while (count > 0) {
			if (!mailItem) {
				mailItem = Item::CreateItem(ITEM_PARCEL, 1);
			}

			uint16_t stackAmount = std::min<uint16_t>(100, count);
			g_game.internalAddItem(mailItem->getContainer(), Item::CreateItem(itemId, stackAmount));
			count -= stackAmount;
		}
	}

	player->onGainExperience(task->experience, nullptr);
	player->addStorageValue(task->storageId, 1);
	if (mailItem) {
		g_game.internalAddItem(player, mailItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
	}

	auto itCompletedTask = m_completedTasks.find(taskName);
	if (itCompletedTask == m_completedTasks.end()) {
		m_completedTasks.emplace(taskName, 1);
	}
	else {
		itCompletedTask->second++;
	}

	if (task->cooldown > 0) {
		task->currentCooldown = OTSYS_TIME() + (task->cooldown * 1000);
	}
	else {
		removeTask(taskName);
	}

	return RET_TASK_NO_ERROR;
}

ReturnTaskMessages PlayerTasks::selectTask(Player* player, const std::string& taskName)
{
	if (!canTakeTask(player, taskName)) {
		return RET_TASK_LIMIT_HAS_BEEN_REACHED;
	}

	if (hasActiveTask(taskName)) {
		return RET_TASK_ALREADY_TAKEN;
	}

	addTask(PlayerTasksData::getInstance()->getTaskByName(taskName), "", 0, 0);
	return RET_TASK_NO_ERROR;
}

ReturnTaskMessages PlayerTasks::cancelTask(const std::string& taskName)
{
	if (!hasActiveTask(taskName)) {
		return RET_TASK_IS_NOT_ACTIVE;
	}

	TaskData* task = getTask(taskName);
	if (!task || task->isCompleted()) {
		return RET_TASK_IS_COMPLETED;
	}

	removeTask(taskName);
	return RET_TASK_NO_ERROR;
}

void PlayerTasks::removeTask(const std::string& name)
{
	for(auto itTask = m_activeTasks.begin(); itTask != m_activeTasks.end(); ++itTask) {
		if (name == itTask->first) {
			m_activeTasks.erase(itTask);
			break;
		}
	}
}

void PlayerTasks::addTask(const TaskData* data, const std::string& monsterName, uint32_t kills, uint64_t cooldown)
{
	if (monsterName.empty()) {
		// Select the task
		m_activeTasks.emplace_back(data->className, TaskData(data, kills, cooldown));
	}
	else {
		// Load the task
		TaskData* task = getTask(data->className);
		if (!task) {
			m_activeTasks.emplace_back(data->className, TaskData(data, monsterName, kills, cooldown));
		}
		else {
			task->updateTask(monsterName, kills);
		}
	}
}

uint32_t PlayerTasks::getTaskKills(const std::string& taskName, const std::string& monsterName)
{
	TaskData* task = getTask(taskName);
	if (!task) {
		return 0;
	}

	return task->getTaskKills(monsterName);
}

uint64_t PlayerTasks::getCooldown(const std::string& taskName)
{
	TaskData* task = getTask(taskName);
	if (!task) {
		return 0;
	}

	if (task->currentCooldown > 0 && task->currentCooldown < static_cast<uint64_t>(OTSYS_TIME())) {
		removeTask(taskName);
		return 0;
	}

	return task->currentCooldown;
}

void PlayerTasks::addTask(const std::string& taskName, const std::string& monsterName, uint32_t kills, uint64_t cooldown)
{
	addTask(PlayerTasksData::getInstance()->getTaskByName(taskName), monsterName, kills, cooldown);
}

void PlayerTasks::addCompletedTask(const std::string& taskName, uint16_t count)
{
	m_completedTasks.emplace(taskName, count);
}

void PlayerTasks::updateTask(Player* player, const std::string& monsterName, bool lastHit)
{
	if (lastHit && !PlayerTasksData::getInstance()->m_taskCountForMostDamage) {
		return;
	}

	for (auto& [className, taskData] : m_activeTasks) {
		if (taskData.isCompleted()) {
			// Task is already finished
			continue;
		}

		if (!taskData.canUse(monsterName)) {
			// This monster does not belong to this task
			continue;
		}

		taskData.updateTask(player, monsterName);
		if (!taskData.isCompleted()) {
			// Task is not finished yet
			continue;
		}

		// Task is finished, yey!
		break;
	}
}

bool PlayerTasks::isActive()
{
	if (m_active) {
		return false;
	}

	m_active = true;
	return true;
}

uint16_t PlayerTasks::getCompletedTasks(const std::string& taskName) const
{
	auto itCompletedTask = m_completedTasks.find(taskName);
	if (itCompletedTask == m_completedTasks.end()) {
		// This task has not been completed yet
		return 0;
	}

	return itCompletedTask->second;
}

bool PlayerTasks::canTakeTask(Player* player, const std::string& taskName) const
{
	if (m_activeTasks.size() >= PlayerTasksData::getInstance()->getMaximumTaskAtOnce(player)) {
		return false;
	}

	TaskData* task = PlayerTasksData::getInstance()->getTaskByName(taskName);
	if (!task) {
		// Task does not exists
		return false;
	}

	if (task->maxRepeatQuantity == 0) {
		// No limitations
		return true;
	}

	return getCompletedTasks(taskName) < task->maxRepeatQuantity;
}

bool PlayerTasks::hasActiveTask(const std::string& name) const
{
	for (auto& [taskName, taskData] : m_activeTasks) {
		if (name == taskName) {
			return true;
		}
	}

	return false;
}

TaskData* PlayerTasks::getTask(const std::string& name)
{
	for (auto& [taskName, taskData] : m_activeTasks) {
		if (name == taskName) {
			return &taskData;
		}
	}

	return nullptr;
}

const TasksList PlayerTasks::getTasks(const Player* player) const
{
	TasksList tasks = m_activeTasks;
	for (auto& [taskName, taskData] : PlayerTasksData::getInstance()->getTasks()) {
		if (taskData.bonusStorageId == 0) {
			continue;
		}

		if (const_cast<Player*>(player)->canUpdateTask(taskData.bonusStorageId)) {
			tasks.push_back({ taskName, taskData });
		}
	}

	return tasks;
}

std::unordered_map<std::string, double> PlayerTasks::getAttributes() const
{
	std::unordered_map<std::string, double> attributes;
	for (auto& [taskName, taskData] : PlayerTasksData::getInstance()->getTasks()) {
		for (auto& [name, value] : taskData.attributes) {
			attributes[name] += value;
		}
	}

	return attributes;
}