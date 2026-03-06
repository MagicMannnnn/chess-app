import { useState } from 'react'
import { ScrollView, StyleSheet, Text, View, TouchableOpacity } from 'react-native'
import Slider from '@react-native-community/slider'

import ChessBoard from '@/components/ChessBoard'
import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'

export default function SandboxScreen() {
  const { settings } = useSettings()
  const [searchDepth, setSearchDepth] = useState(3)
  const [showBestMove, setShowBestMove] = useState(true)

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <View style={styles.controls}>
        <View style={styles.controlRow}>
          <Text style={styles.label}>Search Depth: {searchDepth}</Text>
          <Slider
            style={styles.slider}
            minimumValue={1}
            maximumValue={10}
            step={1}
            value={searchDepth}
            onValueChange={setSearchDepth}
            minimumTrackTintColor={theme.colors.primary}
            maximumTrackTintColor={theme.colors.background.dark}
            thumbTintColor={theme.colors.primary}
          />
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.label}>Show Best Move</Text>
          <TouchableOpacity style={styles.toggle} onPress={() => setShowBestMove(!showBestMove)}>
            <View style={[styles.toggleInner, showBestMove && styles.toggleActive]}>
              <Text style={styles.toggleText}>{showBestMove ? 'ON' : 'OFF'}</Text>
            </View>
          </TouchableOpacity>
        </View>
      </View>

      <ChessBoard
        autoFlip={settings.autoFlipBoard}
        showBestMove={showBestMove}
        searchDepth={searchDepth}
      />
    </ScrollView>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
  },
  content: {
    paddingVertical: 16,
  },
  controls: {
    paddingHorizontal: 16,
    paddingBottom: 16,
    gap: 16,
  },
  controlRow: {
    gap: 8,
  },
  label: {
    fontSize: 16,
    fontWeight: '600',
    color: theme.colors.text.primary,
  },
  slider: {
    width: '100%',
    height: 40,
  },
  toggle: {
    width: 80,
    height: 40,
    borderRadius: 20,
    backgroundColor: theme.colors.background.dark,
    justifyContent: 'center',
    paddingHorizontal: 4,
  },
  toggleInner: {
    width: 36,
    height: 32,
    borderRadius: 16,
    backgroundColor: theme.colors.background.medium,
    justifyContent: 'center',
    alignItems: 'center',
  },
  toggleActive: {
    backgroundColor: theme.colors.primary,
    alignSelf: 'flex-end',
  },
  toggleText: {
    fontSize: 12,
    fontWeight: '600',
    color: '#fff',
  },
})
