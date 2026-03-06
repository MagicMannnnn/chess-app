import { useState } from 'react'
import { ScrollView, StyleSheet, Text, View, TouchableOpacity } from 'react-native'

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
          <View style={styles.sliderContainer}>
            {[1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map((depth) => (
              <TouchableOpacity
                key={depth}
                style={[styles.depthButton, searchDepth === depth && styles.depthButtonActive]}
                onPress={() => setSearchDepth(depth)}
              >
                <Text
                  style={[
                    styles.depthButtonText,
                    searchDepth === depth && styles.depthButtonTextActive,
                  ]}
                >
                  {depth}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
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
  sliderContainer: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
    marginTop: 8,
  },
  depthButton: {
    width: 36,
    height: 36,
    borderRadius: 18,
    backgroundColor: theme.colors.background.medium,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: theme.colors.background.dark,
  },
  depthButtonActive: {
    backgroundColor: theme.colors.primary,
    borderColor: theme.colors.primary,
  },
  depthButtonText: {
    fontSize: 14,
    fontWeight: '600',
    color: theme.colors.text.secondary,
  },
  depthButtonTextActive: {
    color: '#fff',
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
