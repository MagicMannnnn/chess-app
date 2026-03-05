import { StyleSheet, Text, View, Switch } from 'react-native'

import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'

export default function SettingsScreen() {
  const { settings, updateSettings } = useSettings()

  const handleAutoFlipToggle = (value: boolean) => {
    updateSettings({ autoFlipBoard: value })
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
})
