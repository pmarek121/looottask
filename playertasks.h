#pragma once
#include "enums.h"

static constexpr auto MAXIMUM_TASKS_AT_ONCE = 3;

using TaskRewards = std::vector<std::tuple<uint16_t, uint16_t, uint16_t>>;

enum TaskType_t {
	TASKTYPE_MULTITASK,
	TASKTYPE_SINGLETASK
};

enum ReturnTaskMessages_t {
	RET_TASK_NO_ERROR,
	RET_TASK_ALREADY_TAKEN,
	RET_TASK_IS_NOT_ACTIVE,
	RET_TASK_IS_COMPLETED,
	RET_TASK_IS_NOT_COMPLETED,
	RET_TASK_LIMIT_HAS_BEEN_REACHED
};

struct TaskProgress {
	std::vector<std::pair<std::string, Outfit_t>> names;
	TaskRewards rewards;
	double count = 0.;
	double kills = 0.;
	uint8_t id = 0;
	uint64_t experience = 0;

	bool canUse(const std::string& monsterName, uint8_t taskId) const {
		if (taskId != 0 && taskId != id) {
			return false;
		}

		for (auto& [name, outfit] : names) {
			if (monsterName == name) {
				return true;
			}
		}
		return false;
	}
};

struct TaskData
{
	TaskData() {}
	TaskData(const TaskData* data, uint8_t taskId, double kills, uint64_t cooldown) {
		this->className = data->className;
		this->bossName = data->bossName;
		this->experience = data->experience;
		this->storageId = data->storageId;
		this->classOutfit = data->classOutfit;
		this->monsters = data->monsters;
		this->rewards = data->rewards;
		this->cooldown = data->cooldown;
		this->maxRepeatQuantity = data->maxRepeatQuantity;
		this->type = data->type;
		this->currentCooldown = cooldown;
		this->taskId = taskId;
		for (auto& taskProgress : this->monsters) {
			taskProgress.kills = kills;
		}
	}
	TaskData(const TaskData* data, uint8_t taskId, const std::string& name, double kills, uint64_t cooldown) {
		this->className = data->className;
		this->bossName = data->bossName;
		this->experience = data->experience;
		this->storageId = data->storageId;
		this->classOutfit = data->classOutfit;
		this->monsters = data->monsters;
		this->rewards = data->rewards;
		this->cooldown = data->cooldown;
		this->maxRepeatQuantity = data->maxRepeatQuantity;
		this->type = data->type;
		this->currentCooldown = cooldown;
		this->taskId = taskId;
		for (auto& taskProgress : this->monsters) {
			if (taskProgress.canUse(name, taskId)) {
				taskProgress.kills = kills;
				break;
			}
		}
	}

	uint32_t getTaskKills(const std::string& name) const {
		for (auto& taskProgress : this->monsters) {
			if (taskProgress.canUse(name, taskId)) {
				return taskProgress.kills;
			}
		}
		return 0;
	}

	void updateTask(const Player* player, const std::string& name, bool halfEntry);
	void updateTask(const std::string& name, double kills) {
		for (auto& taskProgress : this->monsters) {
			if (taskProgress.kills >= taskProgress.count) {
				continue;
			}

			if (taskProgress.canUse(name, taskId)) {
				taskProgress.kills = kills;
				break;
			}
		}
	}

	bool isCompleted() const {
		for (auto& taskProgress : this->monsters) {
			if (taskId == 0 || taskProgress.id == taskId) {
				if (taskProgress.kills < taskProgress.count) {
					return false;
				}
			}
		}
		return true;
	}

	bool canUse(const std::string& name) const {
		for (auto& taskProgress : this->monsters) {
			if (taskProgress.canUse(name, taskId)) {
				return true;
			}
		}
		return false;
	}

	std::string className;
	std::string bossName;

	std::vector<TaskProgress> monsters;
	TaskRewards rewards;

	uint8_t taskId = 0;

	uint16_t maxRepeatQuantity = 0;

	uint32_t storageId = 0;

	uint64_t experience = 0;
	uint64_t cooldown = 0;
	uint64_t currentCooldown = 0;

	Outfit_t classOutfit;
	TaskType_t type = TASKTYPE_MULTITASK;
};

typedef std::vector<std::pair<std::string, TaskData>> TasksList;

class PlayerTasksData
{
	public:
		static PlayerTasksData* getInstance()
		{
			static PlayerTasksData instance;
			return &instance;
		}

		bool load();
		bool reload();

		const char* getReturnMessage(ReturnTaskMessages_t ret) const;

		uint16_t getMaximumTaskAtOnce(Player* player);

		TaskData* getTaskByName(const std::string& name);

		const TasksList& getTasks() const { return m_playerTasks; }

		int32_t m_globalTaskBonusStorgeId = 0;
		int32_t m_globalTaskBonusGlobalStorgeId = 0;

		bool m_taskCountForMostDamage = true;

	private:
		uint16_t m_maximumTasksAtOnce = 3;
		uint16_t m_maximumPremiumTasksAtOnce = 5;
		TasksList m_playerTasks;
};

typedef std::unordered_map<std::string, uint16_t> CompletedTasks;

class PlayerTasks
{
	public:
		PlayerTasks() {}
		virtual ~PlayerTasks() {}

		void disable() { m_active = false; }
		void updateTask(Player* player, const std::string& monsterName, bool lastHit, bool halfEntry);
		void addTask(const TaskData* data, const std::string& monsterName, uint8_t taskId, double kills, uint64_t cooldown);
		void addTask(const std::string& taskName, const std::string& monsterName, uint8_t taskId, double kills, uint64_t cooldown);
		void addCompletedTask(const std::string& taskName, uint16_t count);
		void removeTask(const std::string& taskName);

		ReturnTaskMessages_t manageTask(Player* player, uint8_t id, const std::string& taskName, uint8_t taskId);

		uint32_t getTaskKills(const std::string& taskName, const std::string& monsterName);

		uint64_t getCooldown(const std::string& taskName);

		bool canTakeTask(Player* player, const std::string& taskName) const;
		bool hasActiveTask(const std::string& taskName) const;
		bool isActive();

		TaskData* getTask(const std::string& name);
		const TasksList getTasks(const Player* player) const;
		const CompletedTasks& getCompletedTasks() const { return m_completedTasks; }

	private:
		uint16_t getCompletedTasks(const std::string& taskName) const;

		ReturnTaskMessages_t completeTask(Player* player, const std::string& taskName);
		ReturnTaskMessages_t selectTask(Player* player, const std::string& taskName, uint8_t taskId);
		ReturnTaskMessages_t cancelTask(const std::string& taskName);

		TasksList m_activeTasks;
		CompletedTasks m_completedTasks;
		bool m_active = false;
};