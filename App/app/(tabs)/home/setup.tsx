import Slider from '@react-native-community/slider'
import { useRouter } from 'expo-router'
import { useState } from 'react'
import { StyleSheet, Text, TouchableOpacity, View } from 'react-native'

import { theme } from '@/constants/theme'

type PlayerMode = 'human-white' | 'human-black' | 'human-both' | 'ai-both'

export default function GameSetupScreen() {
  const router = useRouter()
  const [playerMode, setPlayerMode] = useState<PlayerMode>('human-white')
  const [maxAITime, setMaxAITime] = useState(5) // seconds
  const [maxDepth, setMaxDepth] = useState(5)

  const getPlayerModeLabel = (mode: PlayerMode): string => {
    switch (mode) {
      case 'human-white':
        return 'Human (White) vs AI'
      case 'human-black':
        return 'Human (Black) vs AI'
      case 'human-both':
        return 'Human vs Human'
      case 'ai-both':
        return 'AI vs AI'
    }
  }

  const cyclePlayerMode = () => {
    const modes: PlayerMode[] = ['human-white', 'human-black', 'human-both', 'ai-both']
    const currentIndex = modes.indexOf(playerMode)
    const nextIndex = (currentIndex + 1) % modes.length
    setPlayerMode(modes[nextIndex])
  }

  const startGame = () => {
    router.push({
      pathname: '/home/game',
      params: {
        playerMode,
        maxAITime: maxAITime.toString(),
        maxDepth: maxDepth.toString(),
      },
    })
  }

  return (
    <View style={styles.container}>
      <View style={styles.content}>
        <Text style={styles.title}>New Game</Text>
        <Text style={styles.subtitle}>Configure your game settings</Text>

        <View style={styles.settingsContainer}>
          <View style={styles.setting}>
            <Text style={styles.settingLabel}>Game Mode</Text>
            <TouchableOpacity style={styles.modeButton} onPress={cyclePlayerMode}>
              <Text style={styles.modeButtonText}>{getPlayerModeLabel(playerMode)}</Text>
            </TouchableOpacity>
          </View>

          <View style={styles.setting}>
            <Text style={styles.settingLabel}>Max AI Thinking Time: {maxAITime}s</Text>
            <Text style={styles.settingDescription}>
              AI will make its best move when time runs out
            </Text>
            <Slider
              style={styles.slider}
              minimumValue={1}
              maximumValue={30}
              step={1}
              value={maxAITime}
              onValueChange={setMaxAITime}
              minimumTrackTintColor={theme.colors.primary}
              maximumTrackTintColor={theme.colors.background.dark}
              thumbTintColor={theme.colors.primary}
            />
          </View>

          <View style={styles.setting}>
            <Text style={styles.settingLabel}>Max Search Depth: {maxDepth}</Text>
            <Text style={styles.settingDescription}>
              Maximum depth the AI will search (higher = slower but stronger)
            </Text>
            <Slider
              style={styles.slider}
              minimumValue={1}
              maximumValue={10}
              step={1}
              value={maxDepth}
              onValueChange={setMaxDepth}
              minimumTrackTintColor={theme.colors.primary}
              maximumTrackTintColor={theme.colors.background.dark}
              thumbTintColor={theme.colors.primary}
            />
          </View>
        </View>

        <TouchableOpacity style={styles.startButton} onPress={startGame}>
          <Text style={styles.startButtonText}>Start Game</Text>
        </TouchableOpacity>
      </View>
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
  },
  content: {
    flex: 1,
    padding: 24,
    justifyContent: 'center',
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
    marginBottom: 40,
    textAlign: 'center',
  },
  settingsContainer: {
    gap: 32,
    marginBottom: 40,
  },
  setting: {
    gap: 8,
  },
  settingLabel: {
    fontSize: 18,
    fontWeight: '600',
    color: theme.colors.text.light,
  },
  settingDescription: {
    fontSize: 14,
    color: theme.colors.text.light,
    marginBottom: 4,
  },
  slider: {
    width: '100%',
    height: 40,
  },
  modeButton: {
    backgroundColor: theme.colors.primary,
    paddingHorizontal: 24,
    paddingVertical: 16,
    borderRadius: 12,
    alignItems: 'center',
    marginTop: 8,
  },
  modeButtonText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#fff',
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
