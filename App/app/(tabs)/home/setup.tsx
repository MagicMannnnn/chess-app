import Slider from '@react-native-community/slider'
import { Picker } from '@react-native-picker/picker'
import { useRouter } from 'expo-router'
import { useState } from 'react'
import { ScrollView, StyleSheet, Switch, Text, TouchableOpacity, View } from 'react-native'

import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'

type PlayerType = 'human' | 'ai'
type AIVersion = 'v1' | 'v2'

export default function GameSetupScreen() {
  const router = useRouter()
  const { settings } = useSettings()

  // Player configurations
  const [whitePlayerType, setWhitePlayerType] = useState<PlayerType>('human')
  const [blackPlayerType, setBlackPlayerType] = useState<PlayerType>('ai')

  // AI settings for White
  const [whiteMaxAITime, setWhiteMaxAITime] = useState(5)
  const [whiteMaxDepth, setWhiteMaxDepth] = useState(5)
  const [whiteAIVersion, setWhiteAIVersion] = useState<AIVersion>(settings.aiVersion)

  // AI settings for Black
  const [blackMaxAITime, setBlackMaxAITime] = useState(5)
  const [blackMaxDepth, setBlackMaxDepth] = useState(5)
  const [blackAIVersion, setBlackAIVersion] = useState<AIVersion>(settings.aiVersion)

  // Chess clock settings
  const [useChessClock, setUseChessClock] = useState(false)
  const [clockTimeMinutes, setClockTimeMinutes] = useState(10)

  const startGame = () => {
    router.push({
      pathname: '/home/game',
      params: {
        whitePlayerType,
        blackPlayerType,
        whiteMaxAITime: whiteMaxAITime.toString(),
        whiteMaxDepth: whiteMaxDepth.toString(),
        whiteAIVersion,
        blackMaxAITime: blackMaxAITime.toString(),
        blackMaxDepth: blackMaxDepth.toString(),
        blackAIVersion,
        useChessClock: useChessClock.toString(),
        clockTimeMinutes: clockTimeMinutes.toString(),
      },
    })
  }

  return (
    <View style={styles.container}>
      <ScrollView style={styles.scrollView} contentContainerStyle={styles.content}>
        <Text style={styles.title}>New Game</Text>
        <Text style={styles.subtitle}>Configure your game settings</Text>

        <View style={styles.settingsContainer}>
          {/* White Player Setup */}
          <View style={styles.playerSection}>
            <Text style={styles.playerTitle}>⚪ White Player</Text>

            <View style={styles.toggleRow}>
              <Text style={styles.toggleLabel}>AI Player</Text>
              <Switch
                value={whitePlayerType === 'ai'}
                onValueChange={(value) => setWhitePlayerType(value ? 'ai' : 'human')}
                trackColor={{ false: theme.colors.background.dark, true: theme.colors.primary }}
                thumbColor="#fff"
              />
            </View>

            {whitePlayerType === 'ai' && (
              <View style={styles.aiOptions}>
                <View style={styles.setting}>
                  <Text style={styles.settingLabel}>Max Thinking Time: {whiteMaxAITime}s</Text>
                  <Text style={styles.settingDescription}>
                    AI will make its best move when time runs out
                  </Text>
                  <Slider
                    style={styles.slider}
                    minimumValue={1}
                    maximumValue={30}
                    step={1}
                    value={whiteMaxAITime}
                    onValueChange={setWhiteMaxAITime}
                    minimumTrackTintColor={theme.colors.primary}
                    maximumTrackTintColor={theme.colors.background.dark}
                    thumbTintColor={theme.colors.primary}
                  />
                </View>

                <View style={styles.setting}>
                  <Text style={styles.settingLabel}>Max Search Depth: {whiteMaxDepth}</Text>
                  <Text style={styles.settingDescription}>
                    Maximum depth the AI will search (higher = slower but stronger)
                  </Text>
                  <Slider
                    style={styles.slider}
                    minimumValue={1}
                    maximumValue={10}
                    step={1}
                    value={whiteMaxDepth}
                    onValueChange={setWhiteMaxDepth}
                    minimumTrackTintColor={theme.colors.primary}
                    maximumTrackTintColor={theme.colors.background.dark}
                    thumbTintColor={theme.colors.primary}
                  />
                </View>

                <View style={styles.setting}>
                  <Text style={styles.settingLabel}>AI Engine Version</Text>
                  <Text style={styles.settingDescription}>v1 = balanced, v2 = material-first</Text>
                  <Picker
                    selectedValue={whiteAIVersion}
                    onValueChange={(value) => setWhiteAIVersion(value as AIVersion)}
                    style={styles.picker}
                  >
                    <Picker.Item label="v1 (Balanced)" value="v1" />
                    <Picker.Item label="v2 (Material-First)" value="v2" />
                  </Picker>
                </View>
              </View>
            )}
          </View>

          {/* Black Player Setup */}
          <View style={styles.playerSection}>
            <Text style={styles.playerTitle}>⚫ Black Player</Text>

            <View style={styles.toggleRow}>
              <Text style={styles.toggleLabel}>AI Player</Text>
              <Switch
                value={blackPlayerType === 'ai'}
                onValueChange={(value) => setBlackPlayerType(value ? 'ai' : 'human')}
                trackColor={{ false: theme.colors.background.dark, true: theme.colors.primary }}
                thumbColor="#fff"
              />
            </View>

            {blackPlayerType === 'ai' && (
              <View style={styles.aiOptions}>
                <View style={styles.setting}>
                  <Text style={styles.settingLabel}>Max Thinking Time: {blackMaxAITime}s</Text>
                  <Text style={styles.settingDescription}>
                    AI will make its best move when time runs out
                  </Text>
                  <Slider
                    style={styles.slider}
                    minimumValue={1}
                    maximumValue={30}
                    step={1}
                    value={blackMaxAITime}
                    onValueChange={setBlackMaxAITime}
                    minimumTrackTintColor={theme.colors.primary}
                    maximumTrackTintColor={theme.colors.background.dark}
                    thumbTintColor={theme.colors.primary}
                  />
                </View>

                <View style={styles.setting}>
                  <Text style={styles.settingLabel}>Max Search Depth: {blackMaxDepth}</Text>
                  <Text style={styles.settingDescription}>
                    Maximum depth the AI will search (higher = slower but stronger)
                  </Text>
                  <Slider
                    style={styles.slider}
                    minimumValue={1}
                    maximumValue={10}
                    step={1}
                    value={blackMaxDepth}
                    onValueChange={setBlackMaxDepth}
                    minimumTrackTintColor={theme.colors.primary}
                    maximumTrackTintColor={theme.colors.background.dark}
                    thumbTintColor={theme.colors.primary}
                  />
                </View>

                <View style={styles.setting}>
                  <Text style={styles.settingLabel}>AI Engine Version</Text>
                  <Text style={styles.settingDescription}>v1 = balanced, v2 = material-first</Text>
                  <Picker
                    selectedValue={blackAIVersion}
                    onValueChange={(value) => setBlackAIVersion(value as AIVersion)}
                    style={styles.picker}
                  >
                    <Picker.Item label="v1 (Balanced)" value="v1" />
                    <Picker.Item label="v2 (Material-First)" value="v2" />
                  </Picker>
                </View>
              </View>
            )}
          </View>

          {/* Chess Clock Setup */}
          <View style={styles.playerSection}>
            <View style={styles.toggleRow}>
              <Text style={styles.playerTitle}>⏱️ Chess Clock</Text>
              <Switch
                value={useChessClock}
                onValueChange={setUseChessClock}
                trackColor={{ false: theme.colors.background.dark, true: theme.colors.primary }}
                thumbColor="#fff"
              />
            </View>

            {useChessClock && (
              <View style={styles.aiOptions}>
                <View style={styles.setting}>
                  <Text style={styles.settingLabel}>Time per Player: {clockTimeMinutes} min</Text>
                  <Text style={styles.settingDescription}>
                    Clock starts after White's first move. Game ends if time runs out.
                  </Text>
                  <Slider
                    style={styles.slider}
                    minimumValue={1}
                    maximumValue={60}
                    step={1}
                    value={clockTimeMinutes}
                    onValueChange={setClockTimeMinutes}
                    minimumTrackTintColor={theme.colors.primary}
                    maximumTrackTintColor={theme.colors.background.dark}
                    thumbTintColor={theme.colors.primary}
                  />
                </View>
              </View>
            )}
          </View>
        </View>

        <TouchableOpacity style={styles.startButton} onPress={startGame}>
          <Text style={styles.startButtonText}>Start Game</Text>
        </TouchableOpacity>
      </ScrollView>
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
  },
  scrollView: {
    flex: 1,
  },
  content: {
    padding: 24,
    paddingBottom: 40,
  },
  title: {
    fontSize: 32,
    fontWeight: 'bold',
    color: theme.colors.text.light,
    marginBottom: 8,
    textAlign: 'center',
  },
  subtitle: {
    fontSize: 16,
    color: theme.colors.text.light,
    marginBottom: 32,
    textAlign: 'center',
  },
  settingsContainer: {
    gap: 24,
    marginBottom: 32,
  },
  playerSection: {
    backgroundColor: theme.colors.background.dark,
    borderRadius: 16,
    padding: 20,
    gap: 16,
  },
  playerTitle: {
    fontSize: 22,
    fontWeight: 'bold',
    color: theme.colors.text.dark,
  },
  toggleRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  toggleLabel: {
    fontSize: 16,
    fontWeight: '600',
    color: theme.colors.text.dark,
  },
  aiOptions: {
    gap: 16,
    marginTop: 8,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: theme.colors.background.light,
  },
  setting: {
    gap: 8,
  },
  settingLabel: {
    fontSize: 16,
    fontWeight: '600',
    color: theme.colors.text.dark,
  },
  settingDescription: {
    fontSize: 13,
    color: theme.colors.text.dark,
    opacity: 0.8,
    marginBottom: 4,
  },
  slider: {
    width: '100%',
    height: 40,
  },
  picker: {
    width: '100%',
    backgroundColor: theme.colors.background.dark,
    borderRadius: 8,
    marginTop: 4,
  },
  startButton: {
    backgroundColor: theme.colors.primary,
    paddingHorizontal: 32,
    paddingVertical: 18,
    borderRadius: 12,
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.25,
    shadowRadius: 3.84,
    elevation: 5,
  },
  startButtonText: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#fff',
  },
})
