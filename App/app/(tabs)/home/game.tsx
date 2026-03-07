import { useLocalSearchParams } from 'expo-router'
import { useCallback, useMemo, useState } from 'react'
import { StyleSheet, View } from 'react-native'

import ChessBoard from '@/components/ChessBoard'
import { useSettings } from '@/contexts/SettingsContext'
import { Color } from '@/lib/chess'

type PlayerMode = 'human-white' | 'human-black' | 'human-both' | 'ai-both'

export default function GameScreen() {
  const { settings } = useSettings()
  const params = useLocalSearchParams()

  const playerMode = (params.playerMode as PlayerMode) || 'human-white'
  const maxAITime = parseInt(params.maxAITime as string, 10) || 5
  const maxDepth = parseInt(params.maxDepth as string, 10) || 5

  const [currentPlayer, setCurrentPlayer] = useState<Color>(Color.WHITE)

  const handleCurrentPlayerChange = useCallback((player: Color) => {
    setCurrentPlayer(player)
  }, [])

  // Check if current player is AI
  const isAITurn = useMemo(() => {
    if (playerMode === 'ai-both') return true
    if (playerMode === 'human-both') return false
    if (playerMode === 'human-white') return currentPlayer === Color.BLACK
    if (playerMode === 'human-black') return currentPlayer === Color.WHITE
    return false
  }, [playerMode, currentPlayer])

  return (
    <View style={styles.container}>
      <ChessBoard
        autoFlip={settings.autoFlipBoard}
        searchDepth={maxDepth}
        maxSearchTime={maxAITime * 1000}
        playerMode={playerMode}
        isAIEnabled={isAITurn}
        onCurrentPlayerChange={handleCurrentPlayerChange}
        hideBestMove
      />
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
})
