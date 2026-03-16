import { StyleSheet, Text, View, Switch } from 'react-native'
import { Picker } from '@react-native-picker/picker'

import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'

export default function SettingsScreen() {
  const { settings, updateSettings } = useSettings()

  const handleAutoFlipToggle = (value: boolean) => {
    updateSettings({ autoFlipBoard: value })
  }

  const handleAIVersionChange = (value: 'v1' | 'v2') => {
    updateSettings({ aiVersion: value })
  }

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      <View style={styles.settingsContainer}>
        <View style={styles.settingRow}>
          <View style={styles.settingInfo}>
            <Text style={styles.settingLabel}>Auto-Flip Board</Text>
            <Text style={styles.settingDescription}>
              Automatically flip the board after each move
            </Text>
          </View>
          <Switch
            value={settings.autoFlipBoard}
            onValueChange={handleAutoFlipToggle}
            trackColor={{ false: theme.colors.border.light, true: theme.colors.primary }}
            thumbColor="#ffffff"
          />
        </View>

        <View style={styles.settingRow}>
          <View style={styles.settingInfo}>
            <Text style={styles.settingLabel}>AI Engine Version</Text>
            <Text style={styles.settingDescription}>
              Choose between v1 (balanced) and v2 (material-first)
            </Text>
          </View>
          <Picker
            selectedValue={settings.aiVersion}
            onValueChange={handleAIVersionChange}
            style={styles.picker}
          >
            <Picker.Item label="v1" value="v1" />
            <Picker.Item label="v2" value="v2" />
          </Picker>
        </View>
      </View>
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
    paddingTop: theme.spacing.xl,
  },
  title: {
    fontSize: theme.fontSize.xxl,
    fontWeight: 'bold',
    color: theme.colors.text.light,
    paddingHorizontal: theme.spacing.lg,
    marginBottom: theme.spacing.xl,
  },
  settingsContainer: {
    paddingHorizontal: theme.spacing.lg,
  },
  settingRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingVertical: theme.spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: theme.colors.border.light,
  },
  settingInfo: {
    flex: 1,
    marginRight: theme.spacing.md,
  },
  settingLabel: {
    fontSize: theme.fontSize.md,
    fontWeight: '600',
    color: theme.colors.text.light,
    marginBottom: theme.spacing.xs,
  },
  settingDescription: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.secondary,
  },
  picker: {
    width: 120,
    height: 50,
  },
})
