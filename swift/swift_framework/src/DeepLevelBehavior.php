<?php

/**
 * DeepLevelBehavior
 */
class DeepLevelBehavior extends SecondLevelBehavior {

  /**
   * Build the Deepest level navigation.
   */
  public function build() {
    $fullMenu = parent::build();
    return $fullMenu;
  }

}
